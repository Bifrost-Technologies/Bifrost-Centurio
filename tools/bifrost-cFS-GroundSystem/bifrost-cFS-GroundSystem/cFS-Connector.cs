using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace bifrost_cFS_GroundSystem
{
    // Minimal cFS connector: send CCSDS commands and receive telemetry (TO_LAB on UDP 1235)
    public sealed class CfsConnector : IDisposable
    {
        public const int DefaultCmdPort = 1234;   // CI_LAB receive port
        public const int DefaultTlmPort = 1235;   // TO_LAB telemetry port

        private readonly UdpClient _cmd;
        private readonly UdpClient _tlm;
        private IPEndPoint _cmdEndpoint;
    private CancellationTokenSource? _rxCts;
    private long _rxCount = 0;

        // Header v2 offsets used by NASA GroundSystem
        // TLM header additional offset is handled by consumer; commands need pri/sec offsets
        private readonly byte _cmdOffsetPri = 4;
        private readonly byte _cmdOffsetSec = 4;

        public event EventHandler<TelemetryReceivedEventArgs>? TelemetryReceived;
    public event EventHandler<LogEventArgs>? Log;

        public CfsConnector(string host = "192.168.1.212", int cmdPort = DefaultCmdPort, int tlmPort = DefaultTlmPort)
        {
            _cmd = new UdpClient(AddressFamily.InterNetwork);
            _tlm = new UdpClient(AddressFamily.InterNetwork);
            _tlm.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            _tlm.EnableBroadcast = true;
            _tlm.Client.Bind(new IPEndPoint(IPAddress.Any, tlmPort));
            _cmdEndpoint = new IPEndPoint(IPAddress.Parse(host), cmdPort);
            OnLog("info", $"CfsConnector initialized. Host={host} CmdPort={cmdPort} TlmPort={tlmPort}");
            try { OnLog("info", $"Telemetry socket bound to {_tlm.Client.LocalEndPoint}"); } catch { }
        }

        // Start listening for telemetry (TO_LAB 1235). Raises TelemetryReceived for each datagram.
        public void StartTelemetry()
        {
            _rxCts = new CancellationTokenSource();
            OnLog("info", "StartTelemetry: receive loop starting");
            _ = Task.Run(async () =>
            {
                var ct = _rxCts!.Token;
                while (!ct.IsCancellationRequested)
                {
                    try
                    {
                        var result = await _tlm.ReceiveAsync(ct).ConfigureAwait(false);
                        // Parse APID (first 2 bytes, big-endian Stream ID)
                        if (result.Buffer.Length >= 2)
                        {
                            ushort streamId = BinaryPrimitives.ReadUInt16BigEndian(result.Buffer.AsSpan(0, 2));
                            string apidHex = $"0x{streamId:X}";
                            TelemetryReceived?.Invoke(this, new TelemetryReceivedEventArgs(apidHex, result.Buffer, result.RemoteEndPoint));
                            var cnt = Interlocked.Increment(ref _rxCount);
                            if (cnt == 1 || (cnt % 100) == 0)
                            {
                                OnLog("info", $"Received tlm #{cnt}: {result.Buffer.Length} bytes from {result.RemoteEndPoint} APID={apidHex}");
                            }
                        }
                    }
                    catch (OperationCanceledException)
                    {
                        OnLog("info", "Telemetry receive loop canceled");
                        break;
                    }
                    catch(Exception ex) 
                    {
                        OnLog("error", "Telemetry receive error", ex);
                        // swallow and continue
                    }
                }
            });
        }

        public void StopTelemetry()
        {
            _rxCts?.Cancel();
            OnLog("info", "StopTelemetry: cancel requested");
        }

        // Send a no-parameter command (e.g., NOOP/RESET). pktId is e.g. "0x1806"; endian "LE" or "BE".
        public Task<bool> SendCommandAsync(string pktIdHex, byte cmdCode, string endian = "LE", CancellationToken ct = default)
            => SendCommandAsync(pktIdHex, cmdCode, Array.Empty<byte>(), endian, ct);

        // Send a command with a pre-built payload (already packed, correct endian)
        public async Task<bool> SendCommandAsync(string pktIdHex, byte cmdCode, byte[] payload, string endian = "LE", CancellationToken ct = default)
        {
            var payloadBytes = payload ?? Array.Empty<byte>();
            // Assemble CCSDS primary + cFE sec hdr (v2 offsets), checksum like MiniCmdUtil (XOR of bytes)
            ushort pktId = Convert.ToUInt16(pktIdHex.StartsWith("0x", StringComparison.OrdinalIgnoreCase) ? pktIdHex[2..] : pktIdHex, 16);
            byte[] pri = new byte[6];
            System.Buffers.Binary.BinaryPrimitives.WriteUInt16BigEndian(pri.AsSpan(0, 2), pktId);
            System.Buffers.Binary.BinaryPrimitives.WriteUInt16BigEndian(pri.AsSpan(2, 2), 0xC000);

            byte[] sec = new byte[2];

            int totalLen = pri.Length + _cmdOffsetPri + sec.Length + _cmdOffsetSec + payloadBytes.Length;
            // CCSDS primary length is (total_bytes - 7) and is big-endian
            System.Buffers.Binary.BinaryPrimitives.WriteUInt16BigEndian(pri.AsSpan(4, 2), (ushort)(totalLen - 7));

            var packet = new byte[totalLen];
            int p = 0;
            pri.CopyTo(packet, p); p += pri.Length;
            p += _cmdOffsetPri;
            packet[p++] = cmdCode;
            int checksumIndex = p; packet[p++] = 0x00;
            p += _cmdOffsetSec;
            if (payloadBytes.Length > 0)
            {
                Buffer.BlockCopy(payloadBytes, 0, packet, p, payloadBytes.Length);
                p += payloadBytes.Length;
            }

            byte checksum = 0xFF;
            foreach (var b in packet) checksum ^= b;
            packet[checksumIndex] = checksum;

            // UdpClient.SendAsync(byte[], int, IPEndPoint) doesn't accept a CancellationToken, keep it simple
            int sent = await _cmd.SendAsync(packet, packet.Length, _cmdEndpoint).ConfigureAwait(false);
            bool ok = sent == packet.Length;
            OnLog(ok ? "info" : "warn", $"SendCommand: pktId={pktIdHex} cc={cmdCode} bytes={packet.Length} checksum=0x{packet[checksumIndex]:X2} result={(ok ? "sent" : "partial")} ");
            return ok;
        }

        // Helpers to pack parameters matching MiniCmdUtil’s types
        public static byte[] PackUInt8(byte v) => new[] { v };
        public static byte[] PackInt8(sbyte v) => new[] { unchecked((byte)v) };
        public static byte[] PackUInt16(ushort v, bool bigEndian) { var b = new byte[2]; if (bigEndian) BinaryPrimitives.WriteUInt16BigEndian(b, v); else BinaryPrimitives.WriteUInt16LittleEndian(b, v); return b; }
        public static byte[] PackInt16(short v, bool bigEndian) => PackUInt16(unchecked((ushort)v), bigEndian);
        public static byte[] PackUInt32(uint v, bool bigEndian) { var b = new byte[4]; if (bigEndian) BinaryPrimitives.WriteUInt32BigEndian(b, v); else BinaryPrimitives.WriteUInt32LittleEndian(b, v); return b; }
        public static byte[] PackInt32(int v, bool bigEndian) => PackUInt32(unchecked((uint)v), bigEndian);
        public static byte[] PackUInt64(ulong v, bool bigEndian) { var b = new byte[8]; if (bigEndian) BinaryPrimitives.WriteUInt64BigEndian(b, v); else BinaryPrimitives.WriteUInt64LittleEndian(b, v); return b; }
        public static byte[] PackInt64(long v, bool bigEndian) => PackUInt64(unchecked((ulong)v), bigEndian);
        public static byte[] PackFloat(float v, bool bigEndian) { Span<byte> b = stackalloc byte[4]; System.Runtime.InteropServices.MemoryMarshal.Write(b, in v); if (BitConverter.IsLittleEndian == bigEndian) b.Reverse(); return b.ToArray(); }
        public static byte[] PackDouble(double v, bool bigEndian) { Span<byte> b = stackalloc byte[8]; System.Runtime.InteropServices.MemoryMarshal.Write(b, in v); if (BitConverter.IsLittleEndian == bigEndian) b.Reverse(); return b.ToArray(); }
        public static byte[] PackFixedString(string s, int len)
        {
            var buf = new byte[len];
            var strBytes = Encoding.ASCII.GetBytes(s);
            Array.Copy(strBytes, 0, buf, 0, Math.Min(len, strBytes.Length));
            return buf;
        }

        public void Dispose()
        {
            try { _rxCts?.Cancel(); } catch { }
            _tlm.Dispose();
            _cmd.Dispose();
        }

        private void OnLog(string level, string message, Exception? ex = null)
        {
            try
            {
                Log?.Invoke(this, new LogEventArgs(level, message, ex));
            }
            catch { /* do not let logging throw */ }
            // also write to console as a fallback for developers
            try
            {
                if (ex != null) Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] {level.ToUpperInvariant()}: {message} :: {ex}");
                else Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] {level.ToUpperInvariant()}: {message}");
            }
            catch { }
        }
    }

    public sealed class TelemetryReceivedEventArgs : EventArgs
    {
        public string StreamIdHex { get; }
        public byte[] Buffer { get; }
        public IPEndPoint RemoteEndpoint { get; }
        public TelemetryReceivedEventArgs(string streamIdHex, byte[] buffer, IPEndPoint ep)
            => (StreamIdHex, Buffer, RemoteEndpoint) = (streamIdHex, buffer, ep);
    }

    public sealed class LogEventArgs : EventArgs
    {
        public string Level { get; }
        public string Message { get; }
        public Exception? Exception { get; }
        public LogEventArgs(string level, string message, Exception? exception = null)
            => (Level, Message, Exception) = (level, message, exception);
    }
}

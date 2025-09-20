using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Maui.Controls;
using Microsoft.Maui.Graphics;

namespace bifrost_cFS_GroundSystem.Pages
{
    public partial class TelemetryPage : ContentPage
    {
        // Centurio Nav MIDs and CCs
        private const string CN_CMD_MID = "0x1892";   // command MID
        private const string CN_SENDHK_MID = "0x1893"; // send HK MID
        private const byte CC_NOOP = 0;
        private const byte CC_RESET = 1;
        private const byte CC_SET_MODE = 2;
        private const byte CC_SET_TARGET_POS = 3;
        private const byte CC_SET_TARGET_VEL = 4;
        private const byte CC_SET_TARGET_ATT = 5;
        private const byte CC_SET_THROTTLE = 6;
    // TO_LAB constants to auto-enable telemetry on Start
    private const string TO_LAB_CMD_MID = "0x1880";
    private const byte TO_LAB_OUTPUT_ENABLE_CC = 6;
        private CfsConnector? _cfs;
        public ObservableCollection<TelemetryItem> TelemetryItems { get; } = new();
        public ObservableCollection<LogItem> LogItems { get; } = new();
        private DateTime _lastPkt = DateTime.MinValue;
        private int _frameCount = 0;
        private IDispatcherTimer? _statusTimer;
    private DateTime _startedAt = DateTime.MinValue;
    private bool _noTlmWarned = false;
        private int? _cnHdrOffset = null;    // detected telemetry header offset for Centurio Nav
        private bool? _cnBigEndian = null;   // detected endianness for Centurio Nav floats/doubles

        public TelemetryPage()
        {
            InitializeComponent();
            BindingContext = this;
            _statusTimer = Dispatcher.CreateTimer();
            _statusTimer.Interval = TimeSpan.FromSeconds(1);
            _statusTimer.Tick += (_, __) => UpdateLinkStatus();
            _statusTimer.Start();
        }

        private void EnsureConnector()
        {
            if (_cfs == null)
            {
                _cfs = new CfsConnector(HostEntry.Text?.Trim() ?? "127.0.0.1");
                _cfs.TelemetryReceived += OnTelemetryReceived;
                _cfs.Log += OnConnectorLog;
            }
        }

        private void OnStartTlmClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            _cfs!.StartTelemetry();
            UpdateLinkStatus();
            AppendLog("client", "Start Tlm requested");
            _startedAt = DateTime.Now;
            _noTlmWarned = false;
            _ = AutoEnableTelemetryAsync();
        }

        private void OnStopTlmClicked(object sender, EventArgs e)
        {
            _cfs?.StopTelemetry();
            UpdateLinkStatus();
            AppendLog("client", "Stop Tlm requested");
        }

        private async void OnSendCmdClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            var pktId = CmdPktIdEntry.Text?.Trim() ?? "0x1801";
            var codeText = CmdCodeEntry.Text?.Trim() ?? "0";
            if (!byte.TryParse(codeText, out var cc)) cc = 0;
            AppendLog("client", $"Send Cmd: pktId={pktId} cc={cc}");
            var ok = await _cfs!.SendCommandAsync(pktId, cc);
            if (!ok) await DisplayAlert("Send", "Failed to send command", "OK");
        }

        private async void OnNoopClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            AppendLog("client", "NOOP");
            await _cfs!.SendCommandAsync(CN_CMD_MID, CC_NOOP);
        }

        private async void OnResetClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            AppendLog("client", "RESET");
            await _cfs!.SendCommandAsync(CN_CMD_MID, CC_RESET);
        }

        private async void OnSendHkClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            AppendLog("client", "Send HK");
            await _cfs!.SendCommandAsync(CN_SENDHK_MID, 0);
        }

        private async void OnSetModeClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            if (!byte.TryParse(ModeEntry.Text?.Trim(), out var mode)) { await DisplayAlert("Mode", "Invalid mode (0-255)", "OK"); return; }
            AppendLog("client", $"Set Mode {mode}");
            var payload = CfsConnector.PackUInt8(mode);
            await _cfs!.SendCommandAsync(CN_CMD_MID, CC_SET_MODE, payload);
        }

        private async void OnSetTargetPosClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            if (!double.TryParse(LatEntry.Text?.Trim(), out var lat) ||
                !double.TryParse(LonEntry.Text?.Trim(), out var lon) ||
                !float.TryParse(AltEntry.Text?.Trim(), out var alt))
            { await DisplayAlert("Target Pos", "Invalid lat/lon/alt", "OK"); return; }
            AppendLog("client", $"Set Target Pos {lat},{lon},{alt}");
            var le = false; // pack LE like MiniCmdUtil
            var p1 = CfsConnector.PackDouble(lat, le);
            var p2 = CfsConnector.PackDouble(lon, le);
            var p3 = CfsConnector.PackFloat(alt, le);
            var payload = p1.Concat(p2).Concat(p3).ToArray();
            await _cfs!.SendCommandAsync(CN_CMD_MID, CC_SET_TARGET_POS, payload);
        }

        private async void OnSetTargetVelClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            if (!float.TryParse(VelNEntry.Text?.Trim(), out var vn) ||
                !float.TryParse(VelEEntry.Text?.Trim(), out var ve) ||
                !float.TryParse(VelDEntry.Text?.Trim(), out var vd))
            { await DisplayAlert("Target Vel", "Invalid N/E/D", "OK"); return; }
            AppendLog("client", $"Set Target Vel {vn},{ve},{vd}");
            var le = false;
            var payload = CfsConnector.PackFloat(vn, le)
                .Concat(CfsConnector.PackFloat(ve, le))
                .Concat(CfsConnector.PackFloat(vd, le)).ToArray();
            await _cfs!.SendCommandAsync(CN_CMD_MID, CC_SET_TARGET_VEL, payload);
        }

        private async void OnSetTargetAttClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            if (!float.TryParse(YawEntry.Text?.Trim(), out var yaw) ||
                !float.TryParse(PitchEntry.Text?.Trim(), out var pitch) ||
                !float.TryParse(RollEntry.Text?.Trim(), out var roll))
            { await DisplayAlert("Target Att", "Invalid yaw/pitch/roll", "OK"); return; }
            AppendLog("client", $"Set Target Att {yaw},{pitch},{roll}");
            var le = false;
            var payload = CfsConnector.PackFloat(yaw, le)
                .Concat(CfsConnector.PackFloat(pitch, le))
                .Concat(CfsConnector.PackFloat(roll, le)).ToArray();
            await _cfs!.SendCommandAsync(CN_CMD_MID, CC_SET_TARGET_ATT, payload);
        }

        private async void OnSetThrottleClicked(object sender, EventArgs e)
        {
            EnsureConnector();
            if (!float.TryParse(ThrottleEntry.Text?.Trim(), out var thr)) { await DisplayAlert("Throttle", "Invalid throttle", "OK"); return; }
            AppendLog("client", $"Set Throttle {thr}%");
            var payload = CfsConnector.PackFloat(thr, false);
            await _cfs!.SendCommandAsync(CN_CMD_MID, CC_SET_THROTTLE, payload);
        }

        private void OnTelemetryReceived(object? sender, TelemetryReceivedEventArgs e)
        {
            MainThread.BeginInvokeOnMainThread(() =>
            {
                var preview = BitConverter.ToString(e.Buffer.Take(32).ToArray()).Replace('-', ' ');
                TelemetryItems.Insert(0, new TelemetryItem
                {
                    Header = $"{DateTime.Now:HH:mm:ss} {e.StreamIdHex} from {e.RemoteEndpoint.Address}",
                    Preview = preview + (e.Buffer.Length > 32 ? " ..." : string.Empty)
                });
                // cap list
                if (TelemetryItems.Count > 200)
                {
                    TelemetryItems.RemoveAt(TelemetryItems.Count - 1);
                }

                _frameCount++;
                _lastPkt = DateTime.Now;
                FrameCountLabel.Text = _frameCount.ToString();
                LastPktTimeLabel.Text = _lastPkt.ToString("HH:mm:ss");
                _noTlmWarned = true; // suppress any pending no-telemetry warning once we get first packet

                // Decode Centurio Nav HK when Stream ID matches 0x894
                // CCSDS primary header is 6 bytes; offsets in spec are relative to full packet start
                // Spec fields begin at offsets listed in cfs-centurio-nav-hk-tlm.txt
                try
                {
                    if (e.StreamIdHex.Equals("0x894", StringComparison.OrdinalIgnoreCase))
                    {
                        var buf = e.Buffer;
                        if (buf.Length >= 66) // ensure we have at least through throttle and spare
                        {
                            // Detect header offset and endianness once
                            if (_cnHdrOffset == null || _cnBigEndian == null)
                            {
                                (int dOff, bool dBe) = DetectCnHdrAndEndian(buf);
                                _cnHdrOffset = dOff;
                                _cnBigEndian = dBe;
                                AppendLog("info", $"Centurio Nav HK decode: hdrOff={dOff}, endian={(dBe ? "BE" : "LE")}");
                            }
                            int off = _cnHdrOffset ?? 16;
                            bool be = _cnBigEndian ?? false;

                            byte cmdErr = buf[off + 12];
                            byte cmdCnt = buf[off + 13];
                            byte status = buf[off + 14];
                            byte fix = buf[off + 15];
                            double lat = ReadDouble(buf, off + 16, be);
                            double lon = ReadDouble(buf, off + 24, be);
                            float alt = ReadFloat(buf, off + 32, be);
                            float vn = ReadFloat(buf, off + 36, be);
                            float ve = ReadFloat(buf, off + 40, be);
                            float vd = ReadFloat(buf, off + 44, be);
                            float yaw = ReadFloat(buf, off + 48, be);
                            float pitch = ReadFloat(buf, off + 52, be);
                            float roll = ReadFloat(buf, off + 56, be);
                            float thr = ReadFloat(buf, off + 60, be);

                            CmdCntLabel.Text = cmdCnt.ToString();
                            ErrCntLabel.Text = cmdErr.ToString();
                            StatusLabel.Text = status.ToString();
                            FixLabel.Text = fix.ToString();
                            LatLabel.Text = lat.ToString("F7");
                            LonLabel.Text = lon.ToString("F7");
                            AltLabel.Text = alt.ToString("F2");
                            VelNLabel.Text = vn.ToString("F2");
                            VelELabel.Text = ve.ToString("F2");
                            VelDLabel.Text = vd.ToString("F2");
                            YawLabel.Text = yaw.ToString("F1");
                            PitchLabel.Text = pitch.ToString("F1");
                            RollLabel.Text = roll.ToString("F1");
                            ThrottleLabel.Text = thr.ToString("F1");
                        }
                    }
                }
                catch { /* ignore per-packet decode errors */ }

                UpdateLinkStatus();
            });
        }

        private void OnConnectorLog(object? sender, LogEventArgs e)
        {
            MainThread.BeginInvokeOnMainThread(() =>
            {
                var line = $"[{DateTime.Now:HH:mm:ss}] {e.Level.ToUpperInvariant()}: {e.Message}";
                if (e.Exception != null)
                {
                    line += $" :: {e.Exception.Message}";
                }
                LogItems.Insert(0, new LogItem { Line = line });
                if (LogItems.Count > 300) LogItems.RemoveAt(LogItems.Count - 1);
            });
        }

        private static float ReadFloat(byte[] buf, int offset, bool bigEndian)
        {
            var span = new ReadOnlySpan<byte>(buf, offset, 4);
            // If system LE and payload BE, reverse; if system BE and payload LE, reverse
            bool needSwap = (BitConverter.IsLittleEndian && bigEndian) || (!BitConverter.IsLittleEndian && !bigEndian);
            if (needSwap)
            {
                Span<byte> tmp = stackalloc byte[4];
                span.CopyTo(tmp);
                tmp.Reverse();
                return BitConverter.ToSingle(tmp);
            }
            return BitConverter.ToSingle(span);
        }

        private static double ReadDouble(byte[] buf, int offset, bool bigEndian)
        {
            var span = new ReadOnlySpan<byte>(buf, offset, 8);
            bool needSwap = (BitConverter.IsLittleEndian && bigEndian) || (!BitConverter.IsLittleEndian && !bigEndian);
            if (needSwap)
            {
                Span<byte> tmp = stackalloc byte[8];
                span.CopyTo(tmp);
                tmp.Reverse();
                return BitConverter.ToDouble(tmp);
            }
            return BitConverter.ToDouble(span);
        }

        private void UpdateLinkStatus()
        {
            // Consider 'Connected' if we received a packet in the last 3 seconds and telemetry is started
            bool receiving = _cfs != null && (DateTime.Now - _lastPkt) < TimeSpan.FromSeconds(3);
            ConnIndicator.Color = receiving ? Colors.LimeGreen : Colors.Red;
            ConnStatusLabel.Text = receiving ? "Connected" : "Disconnected";

            // If we haven't seen any telemetry shortly after starting, log a hint
            if (!receiving && _startedAt != DateTime.MinValue && !_noTlmWarned)
            {
                var elapsed = DateTime.Now - _startedAt;
                if (elapsed > TimeSpan.FromSeconds(5))
                {
                    _noTlmWarned = true;
                    AppendLog("warn", "No telemetry received yet. If cFS runs in WSL/VM, ensure TO_LAB destination IP is your Windows host IP (not 127.0.0.1), and allow UDP 1235 inbound in Windows Firewall.");
                }
            }
        }

        private void AppendLog(string level, string message)
        {
            var line = $"[{DateTime.Now:HH:mm:ss}] {level.ToUpperInvariant()}: {message}";
            LogItems.Insert(0, new LogItem { Line = line });
            if (LogItems.Count > 300) LogItems.RemoveAt(LogItems.Count - 1);
        }

        private static (int off, bool bigEndian) DetectCnHdrAndEndian(byte[] buf)
        {
            // Try common header offsets and both endian options; choose the best score based on plausibility of multiple fields
            int[] offsets = new[] { 0, 4, 6, 8, 12, 14, 16, 18, 20, 24 };
            (int off, bool be, int score) best = (16, false, int.MinValue);
            foreach (var be in new[] { false, true })
            {
                foreach (var off in offsets)
                {
                    if (buf.Length < off + 64) continue;
                    try
                    {
                        int sc = 0;
                        double lat = ReadDouble(buf, off + 16, be);
                        double lon = ReadDouble(buf, off + 24, be);
                        if (lat >= -90 && lat <= 90) sc += 2; else sc -= 2;
                        if (lon >= -180 && lon <= 180) sc += 2; else sc -= 2;
                        float alt = ReadFloat(buf, off + 32, be);
                        if (alt > -5000 && alt < 100000) sc += 1;
                        float yaw = ReadFloat(buf, off + 48, be);
                        float pitch = ReadFloat(buf, off + 52, be);
                        float roll = ReadFloat(buf, off + 56, be);
                        if (yaw > -720 && yaw < 720) sc += 1;
                        if (pitch > -720 && pitch < 720) sc += 1;
                        if (roll > -720 && roll < 720) sc += 1;
                        float thr = ReadFloat(buf, off + 60, be);
                        if (thr >= -10 && thr <= 110) sc += 1;
                        if (sc > best.score)
                        {
                            best = (off, be, sc);
                        }
                    }
                    catch { }
                }
            }
            return (best.off, best.be);
        }

        // Resolve the local IPv4 address that will be used to reach the cFS host.
        // This helps choose the correct dest_IP for TO_LAB when cFS runs on another machine/VM.
        private static string ResolveLocalIPv4ForRemote(string remoteHost, int remotePort)
        {
            try
            {
                using var s = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
                // Use a best-effort connect to let OS routing pick an interface
                s.Connect(remoteHost, remotePort);
                if (s.LocalEndPoint is IPEndPoint lep)
                {
                    return lep.Address.ToString();
                }
            }
            catch { }
            // Fallback to local host
            return "127.0.0.1";
        }

        private async Task AutoEnableTelemetryAsync()
        {
            try
            {
                var host = HostEntry.Text?.Trim();
                if (string.IsNullOrWhiteSpace(host)) host = "127.0.0.1";

                // Pick local IP used to reach the cFS host (port 1235 is fine)
                var localIp = ResolveLocalIPv4ForRemote(host!, 1235);
                var payload = CfsConnector.PackFixedString(localIp, 16);
                AppendLog("client", $"Enable TO_LAB telemetry to {localIp}");
                var ok = await _cfs!.SendCommandAsync(TO_LAB_CMD_MID, TO_LAB_OUTPUT_ENABLE_CC, payload);
                if (!ok)
                {
                    AppendLog("warn", "Failed to send TO_LAB enable telemetry command");
                }
            }
            catch (Exception ex)
            {
                AppendLog("error", $"Auto-enable telemetry exception: {ex.Message}");
            }
        }
    }

    public sealed class TelemetryItem
    {
        public string Header { get; set; } = string.Empty;
        public string Preview { get; set; } = string.Empty;
    }

    public sealed class LogItem
    {
        public string Line { get; set; } = string.Empty;
    }
}

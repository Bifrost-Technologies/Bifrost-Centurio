# Throttle to 65%
python tools/clients/centurio_json_client.py set_throttle --percent 65

# Set mode to 1
python tools/clients/centurio_json_client.py set_mode --mode 1

# Set a position target
python tools/clients/centurio_json_client.py set_target_pos --lat 37.619 --lon -122.374 --alt_m 120

# Set a velocity target (m/s)
python tools/clients/centurio_json_client.py set_target_vel --vn 10 --ve 0 --vd 0

# Set attitude target (deg)
python tools/clients/centurio_json_client.py set_target_att --yaw 45 --pitch 0 --roll 0

# Housekeeping
python tools/clients/centurio_json_client.py noop
python tools/clients/centurio_json_client.py reset
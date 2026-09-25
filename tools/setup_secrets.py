"""Generate local prototype credentials once; never import Haier credentials."""
from pathlib import Path
import secrets
root = Path(__file__).resolve().parents[1]
path = root / 'secrets.yaml'
if not path.exists():
    with path.open('x', encoding='utf-8') as f:
        for key in ('setup_password', 'web_password', 'ota_password'):
            f.write(f'{key}: "{secrets.token_urlsafe(18)}"\n')
        f.write('mqtt_broker: "192.0.2.1"\nmqtt_username: ""\nmqtt_password: ""\n')
    print('Created secrets.yaml with unique passwords; set your MQTT broker before final build.')
else:
    print('Existing secrets.yaml preserved.')

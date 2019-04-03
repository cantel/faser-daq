Dependencies:
```
sudo ansible-playbook install-redis.yml --ask-become-pass
sudo ansible-playbook install-webdeps.yml --ask-become-pass
```

Run test server: 
```
python3 metric-manager.py
```


### Scriptable actions
It is possible to define bash scripts that can be executed on specific tail nodes. For example, take this simple script to open up a new SSH session in Konsole:
```lang=bash
#!/bin/bash

# This script opens an SSH connection to a specified host.
# Usage: ./ssh-open.sh <IP_ADDRESS> <HOSTNAME>

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <IP_ADDRESS> <HOSTNAME>"
    exit 1
fi

IP_ADDRESS="$1"
HOSTNAME="$2"

echo "Opening SSH connection to $HOSTNAME at $IP_ADDRESS..."
# Spawn a new KDE terminal window and run the SSH command

konsole -e bash -c "ssh grenis@$HOSTNAME; exec bash" &

# Note: Replace 'konsole' with your preferred terminal emulator if needed.
```

### Things to consider
For a script to be picked up and runnable, it needs to be marked as executable. Eg. make sure to set your script(s) as executable such as: `chmod +x my-script.sh`


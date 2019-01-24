# Message Slot
Implementing a new mechanism for inter-process communication.

A message slot is a character device file through which processes communicate using multiple message channels. A message slot device can have multiple channels active concurrently, which can be used by different processes.

Once a message slot device file is opened, a process uses ioctl() to specify the id of the message channel it wants to use. It subsequently uses read()/write() to receive/send messages on the channel. In contrast to pipes, a message slot preserves a message until it is overwritten; so the same message can be read multiple times.

Message slots are implemented with a character device driver. Each message slot is a character device file managed by this driver.

## Message Sender
### Command line arguments:
1. argv[1] – message slot file path.
2. argv[2] – the target message channel id. Assume a non-negative integer.
3. argv[3] – the message to pass.

### Flow:
1. Open the specified message slot device file.
2. Set the channel id to the id specified on the command line.
3. Write the specified message to the file.
4. Close the device.
5. Print a status message.

## Message Reader
### Command line argument:
1. argv[1] – message slot file path.
2. argv[2] – the target message channel id. Assume a non-negative integer.
### Flow:
1. Open the specified message slot device file.
2. Set the channel id to the id specified on the command line.
3. Read a message from the device to a buffer.
4. Close the device.
5. Print the message and a status message.

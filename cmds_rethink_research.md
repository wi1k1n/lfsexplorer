### Linux commands to work with files

General/Files commands:

- help - show help message
- mkdir - make directory
- rmdir - remove directory
- cp - copy files and directories
- mv - move files and directories
- rm - remove files
- touch - create file or update it's timestamp (is it needed?)
- find - ?
- pwd - print current working directory
- ls - list directory contents
- cat - print (formatted) file content
- cd - change directory
- head - print first part of the file content
- tail - print last part of the file content
- df - ?
- du - ?

Network commands:

- ping
- curl
- wget
- ifconfig

### Commands format

Each command has the following general structure:
`command_name <args>`

Command name encodes the tool that is to be executed and should correspond to one of the listed above command names (case insensitive). Command arguments are optional and follow the command name (space separated). Each command can not be bigger than `LFSE_SERIAL_BUFFER_LENGTH` length (`256` by default).

### Arguments

Each argument can be one of the following types:

- path/filename - stringified path to the file or directory that contains alphanumeric symbols or on of `{'-', '.', '/'}`
- symbolic flag - a single alphanumeric character used after single `-`
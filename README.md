# LFSExplorer

Easily navigate and manipulate the LittleFS filesystem on your ESP via Serial interface.
Provides toolset necessary for file content and structure operations.

## Toolset

The following commands can be called through Serial:

- **help** - show help message
- **wipe** - delete everything (aka format filesystem)
- **ls** - list directory contents
- **cd** - change directory
- **pwd** - print current working directory
- **mkdir** - make directory
- **mv** - move files and directories
- **rm** - remove files and directories
- **cp** - copy files and directories
- *touch* - **deprecated**
- **tee** - save text from arguments into file
- **cat** - print (formatted) file content out or to file
- **man** - show manual entry for the specified command

## Command format

Each command has the following general structure:
`command <args>`

Command should correspond to one of the listed above (case insensitive).
The arguments is an optional part that goes after command word (space separated).
Each command can be no bigger than `LFSE_SERIAL_BUFFER_LENGTH` length (`256` by default).

### Argument types

Arguments are represented with 3 types:

- path/filename - can contain alphanumeric symbols or one of {'-', '.', '/'}
- flag - starts with symbol '-' followed by alphanumeric symbols + one of {'-', '.'}
- string - escaped with `"` from both ends (`\"`, `\\`, `\n`, `\r`, `\t` can be used inside string to get `"`, `\`, _LF_, _CR_, _TAB_ correspondingly)

#### Flag types

Flags can be of _Literal_ or _Numerical_ types.
Literal flags can be combined to a single multi-literal flag (e.g. `cat -bp file` is equivalent to `cat -b -p file`).
Numerical flags cannot be combined together.

Example commands:

- `write -a file "hello world"` - a single literal flag `-a` is used
- `cat -c5 file` - a numerical flag `-c` is used with value `5`
- `cat -bp -c8 -f1 -l9 file` - two literal flags `-b` and `-p` are used, so are numerical flags `-c`, `-f`, `l` with values `8`, `1`, `9` correspondingly

## TODO List:
- add command `truncate`
- remove `touch` command
- change `tee` behavior: `-f` flag for overwriting data from beginning of the file, default behaivor: append mode
- make `cat` command be able to output to file when using `-o` flag
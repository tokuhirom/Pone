# file

This package provides basic file operations.

## `file.open(Str $filename[, Str $mode) --> File`

open `$filename` with mode `$mode`.

The argument mode points to a string beginning with one of the following sequences (possibly  followed
by additional characters, as described below):

       r      Open text file for reading.  The stream is positioned at the beginning of the file.

       r+     Open for reading and writing.  The stream is positioned at the beginning of the file.

       w      Truncate  file to zero length or create text file for writing.  The stream is positioned at the
              beginning of the file.

       w+     Open for reading and writing.  The file is created if it does not exist, otherwise it is  trun‐
              cated.  The stream is positioned at the beginning of the file.

       a      Open  for  appending  (writing at end of file).  The file is created if it does not exist.  The
              stream is positioned at the end of the file.

       a+     Open for reading and appending (writing at end of file).  The file is created if  it  does  not
              exist.   The  initial  file position for reading is at the beginning of the file, but output is
              always appended to the end of the file.

### RETURN VALUE

Returns `File` object. IF Pone can't open the file set `$!`

(TODO: support autodie)

## `file.fdopen(Int $fd[, Str $mode) --> File`

Open file by fdopen(3).

## `File#read(Int $nbytes) --> Str`

The method `read` reads `$nbytes` bytes of data from the stream.

Read `$buflen` bytes from file. Returns read string.

### RETURN VALUE

Return the number of bytes read.
This number equals the number of bytes transferred.
If an error occurs,  or  the  end  of  the  file  is
reached, the return value is a short item count (or zero).

## `File#write(Str $buf) --> Int`

The method `write` writes `$nbytes` bytes of data to the stream.

### RETURN VALUE

Return the number of bytes written.
If an error occurs,  or  the  end  of  the  file  is
reached, the return value is a short item count (or zero).

## `File#rewind()`

This method sets the ile position indicator for the stream pointed to by stream to the
beginning of the file.

## `File#fileno() --> Int`

examines the argument stream and returns its integer descriptor.

## `File#seek()`


sets the file position indicator for the stream pointed to by stream.  The new
position, measured in bytes, is obtained by adding offset bytes to the position specified  by  whence.
If  whence is set to `SEEK_SET`, `SEEK_CUR`, or `SEEK_END`, the offset is relative to the start of the file,
the current position indicator, or end-of-file, respectively.

## `File#flock(Int $operation)`

apply or remove an advisory lock on an open file.

Apply  or  remove an advisory lock on the open file.
The argument operation is one of the following:

    LOCK_SH  Place a shared lock.  More than one process may hold a shared lock for a given file at  a
            given time.

    LOCK_EX  Place an exclusive lock.  Only one process may hold an exclusive lock for a given file at
            a given time.

    LOCK_UN  Remove an existing lock held by this process.

## `File#eof() --> Bool`

tests the end-of-file indicator for the stream pointed  to  by  stream

## `File#close()`

Close the stream.

## CONSTANTS

### `file.SEEK_SET`
### `file.SEEK_CUR`
### `file.SEEK_END`

### `file.LOCK_EX`
### `file.LOCK_UN`
### `file.LOCK_SH`


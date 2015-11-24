# TempFile

## Temporary directory

pone uses `$*ENV<TMPDIR> // "/tmp"` as a temporary directory.

## FUNCTIONS

### `tempfile()`

Create new temporary file in temporary directory.

This returns instance of TempFile.

## Classes

### `TempFile`

TempFile is instance of temporary file.

#### Methods

##### `$tmpfile.path()`

Get the path of temporary file.

##### `$tmpfile.file()`

Get the file handle of temporary file.



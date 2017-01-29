# libomemo
Implements OMEMO in C.
Input and output are XML strings, so it does not force you to use a certain XML lib.
While the actual protocol functions do not depend on any kind of storage, it comes with a basic implementation in SQLite.

## Dependencies ##
* [Mini-XML](http://www.msweet.org/projects.php?Z3) 
* OpenSSL (which you probably already have)
* If you want to use the SQLite database, you will obviously need that.

## Usage ##
I recommend to simply link it statically. Test cases and makefile demonstrate the general usage. 

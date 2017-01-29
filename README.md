# libomemo
Implements [OMEMO](https://conversations.im/omemo/) in C.

Input and output are XML strings, so it does not force you to use a certain XML lib.
While the actual protocol functions do not depend on any kind of storage, it comes with a basic implementation in SQLite.

It does not handle the double ratchet sessions, just the encryption of the payload.

## Dependencies
* [Mini-XML](http://www.msweet.org/projects.php?Z3) 
* OpenSSL (which you probably already have)
* glib
* If you want to use the SQLite database, you will obviously need that.

I recommend to simply link it statically - there are makefile targets for compilation as a libtool lib. 

## Usage
Basically, there are three data types: messages, devicelists, and bundles.
You can import received the received XML data to work with it, or create them empty. When done with them, they can be exported back to XML for displaying or sending.

The test cases should demonstrate the general usage.

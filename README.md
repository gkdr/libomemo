# libomemo
Implements [OMEMO](https://conversations.im/omemo/) in C.

Input and output are XML strings, so it does not force you to use a certain XML lib.
While the actual protocol functions do not depend on any kind of storage, it comes with a basic implementation in SQLite.

It does not handle the double ratchet sessions, just the encryption of the payload.
Use [axc](https://github.com/gkdr/axc) for that.

## Dependencies
* [Mini-XML](http://www.msweet.org/projects.php?Z3) 
* OpenSSL (which you probably already have)
* glib
* SQLite for the database

Optional: 
* For testing: [cmocka](https://cmocka.org/) (`make test`)
* For the coverage report: [gcovr](https://cmocka.org/) (`make coverage`)

I recommend to simply link it statically - there are makefile targets for compilation as a libtool lib. 

## Usage
Basically, there are three data types: messages, devicelists, and bundles.
You can import received the received XML data to work with it, or create them empty. When done with them, they can be exported back to XML for displaying or sending.

The test cases should demonstrate the general usage.


If a different namespace than the one specified in the XEP is to be used, you can use specify it at compile time. See the makefile for an example.

# libomemo 0.6.2
Implements [OMEMO](https://conversations.im/omemo/) ([XEP-0384](https://xmpp.org/extensions/xep-0384.html)) in C.

Input and output are XML strings, so it does not force you to use a certain XML lib.
While the actual protocol functions do not depend on any kind of storage, it comes with a basic implementation in SQLite.

It deals with devicelists and bundles as well as encrypting the payload, but does not handle the double ratchet sessions for encrypting the key to the payload.
However, you can use my [axc](https://github.com/gkdr/axc) lib for that and easily combine it with this one (or write all the libsignal client code yourself if that is better suited to your needs).

## Dependencies
* [Mini-XML](http://www.msweet.org/projects.php?Z3) (`libmxml-dev`)
* gcrypt (`libgcrypt20-dev`)
* glib (`libglib2.0-dev`)
* SQLite (`libsqlite3-dev`)

Optional: 
* For testing: [cmocka](https://cmocka.org/) (`make test`)
* For the coverage report: [gcovr](https://cmocka.org/) (`make coverage`)

I recommend to simply link it statically - the standard target is a is a static lib (containing PIC) which uses the _compatible_ namespace (i.e. not the one in the XEP). 

## Usage
Basically, there are three data types: messages, devicelists, and bundles.
You can import received the received XML data to work with it, or create them empty. When done with them, they can be exported back to XML for displaying or sending.

The test cases should demonstrate the general usage.


If a different namespace than the one specified in the XEP is to be used, you can use specify it at compile time. See the makefile for an example.

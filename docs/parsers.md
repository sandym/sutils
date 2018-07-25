## `su_saxparser.h`

Simple XML parser, implemented as a wrapper to expat. Just subclass saxparser.

Usage:
```C++
class MyParser : public su::saxparser
{
public:
  MyParser( std::istream &i_stream )
   : su::saxparser( i_stream ){}

  virtual void startDocument()
  {
    // handle start of the document
  }
  virtual void endDocument()
  {
    // handle end of the document
  }
  virtual bool startElement( const su::NameURI &i_nameURI,
    const std::unordered_map<su::NameURI,std::string_view> &i_attribs )
  {
    // handle atart of an element
    return true;
  }
  virtual bool endElement( const su::NameURI &i_nameURI )
  {
    // handle end of an element
    return true;
  }
  virtual bool characters( const std::string_view &i_s )
  {
    // handle data
    return true;
  }
};

```

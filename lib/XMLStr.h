#ifndef __XMLSTR_H__
#define __XMLSTR_H__

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>

// ---------------------------------------------------------------------------
//  This is a simple class that lets us do easy (though not terribly efficient)
//  trancoding of char* data to XMLCh data.
// ---------------------------------------------------------------------------
class XMLStr
{
 public :
  // ----------------------------------------------------------------------
  //  Constructors and Destructor
  // -----------------------------------------------------------------------
  XMLStr(const char* const toTranscode)
    {
      // Call the private transcoding method
      fUnicodeForm = XERCES_CPP_NAMESPACE_QUALIFIER XMLString::transcode(toTranscode);
    }

  ~XMLStr()
    {
      XERCES_CPP_NAMESPACE_QUALIFIER XMLString::release(&fUnicodeForm);
    }


  // -----------------------------------------------------------------------
  //  Getter methods
  // -----------------------------------------------------------------------
  const XMLCh* unicodeForm() const
  {
    return fUnicodeForm;
  }

 private :
  // -----------------------------------------------------------------------
  //  Private data members
  //
  //  fUnicodeForm
  //      This is the Unicode XMLCh format of the string.
  // -----------------------------------------------------------------------
  XMLCh*   fUnicodeForm;
};

#define X(str) XMLStr(str).unicodeForm()


#endif

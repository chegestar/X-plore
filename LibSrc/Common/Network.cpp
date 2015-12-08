//----------------------------
// Multi-platform mobile library
// (c) Lonely Cat Games
//----------------------------

#include <Network.h>
#include <TextUtils.h>

//----------------------------

bool S_content_type::Parse(const char *&cp){

                              //read type
                              // type = discrete-type | composite-type
                              //    discrete-type = "text" | "image" | "audio" | "video" | "application" | extension-token
                              //    composite-type = "message" | "multipart" | extension-token
                              //       extension-token = ietf-token | x-token
                              //          ietf-token = <extension token defined by a standards-track RFC and registered with IANA>
                              //          x-token = <the two characters "X-" or "x-" followed, with no intervening white space, by any token>
                              // subtype = extension-token / iana-token
                              //    iana-token = <publicly-defined extension token; tokens of this form must be registered with IANA as specified in RFC 2048>
   if(text_utils::CheckStringBegin(cp, "text/") || text_utils::CheckStringBegin(cp, "text")){
      type = CONTENT_TEXT;
   }else
   if(text_utils::CheckStringBegin(cp, "image/")){
      type = CONTENT_IMAGE;
   }else
   if(text_utils::CheckStringBegin(cp, "video/")){
      type = CONTENT_VIDEO;
   }else
   if(text_utils::CheckStringBegin(cp, "audio/")){
      type = CONTENT_AUDIO;
   }else
   if(text_utils::CheckStringBegin(cp, "application/")){
      type = CONTENT_APPLICATION;
   }else
   if(text_utils::CheckStringBegin(cp, "multipart/")){
      type = CONTENT_MULTIPART;
   }else
   if(text_utils::CheckStringBegin(cp, "message/"))
      type = CONTENT_MESSAGE;
#ifdef _DEBUG_
   else{
      assert(0);
      return false;
   }
#endif
                              //read subtype
   if(!text_utils::ReadWord(cp, subtype, text_utils::specials_rfc_822))
      return false;
   subtype.ToLower();
   return true;
}

//----------------------------

bool S_content_type::ParseWithCoding(const char *&cp, E_TEXT_CODING &cod){

   cod = COD_DEFAULT;
   if(!Parse(cp))
      return false;

   while(true){
      text_utils::SkipWS(cp);
      if(*cp!=';')
         break;
      ++cp;
      Cstr_c attr, val;
      /*
      if(!text_utils::GetNextAtrrValuePair(cp, attr, val, true))
         break;
      encoding::CharsetToCoding(val, cod);
      */
      text_utils::SkipWS(cp);
      if(!text_utils::ReadToken(cp, attr, " ()<>@,;:\\\"/[]?="))
         break;
      text_utils::SkipWS(cp);
      if(*cp!='=')
         break;
      ++cp;
      text_utils::SkipWS(cp);
      if(!text_utils::ReadQuotedString(cp, val) && !text_utils::ReadToken(cp, val, " ;"))
         break;
      attr.ToLower();
                              //interpret attr/value pair
      if(attr=="charset"){
         val.ToLower();
         encoding::CharsetToCoding(val, cod);
      }
   }
   return true;
}

//----------------------------

bool C_application_http::S_http_header::ReadPhrase(const char *&cp, Cstr_c &str){

   bool ok = false;
   Cstr_c tmp;

   while(text_utils::ReadWord(cp, tmp, "()<>\x7f")){
      str<<tmp;
      text_utils::SkipWS(cp);
      ok = true;
   }
   return ok;
}

//----------------------------

bool C_application_http::S_http_header::ReadHTTPDateTime(const char *&cp, S_date_time &ret_date){

   if(text_utils::ReadDateTime_rfc822_rfc850(cp, ret_date, false))
      return true;
   if(text_utils::ReadDateTime_rfc822_rfc850(cp, ret_date, true))
      return true;
   return ReadDateTime_ANSI_C(cp, ret_date);
}

//----------------------------

void C_application_http::S_http_header::Reset(){

   content = S_content_type();
   date = 0;
   expire_date = 0;
   last_modify_date = 0;
   content_length = 0;
   chunked = false;
   connection_close = false;
   location.Clear();
   http_ver = 0;
   response_code = 0;
   content_encoding = ENCODING_IDENTITY;
   flags = 0;
   content_coding = COD_DEFAULT;
}

//----------------------------

static const char http_keywords[] =
   "http/\0"
                              //general header fields
   "date:\0"
   "last-modified:\0"
   "content-type:\0"
   "content type:\0"
   "content-length:\0"
   "transfer-encoding:\0"
   "connection:\0"
   "conection\0"  //mispelling of connection
   "cache-control:\0"
   "pragma:\0"
   "trailer:\0"
   "upgrade:\0"
   "warning:\0"
                              //response header fields
   "location:\0"
   "set-cookie:\0"
   "expires:\0"
   "content-language:\0"
   "content-encoding:\0"
   "content-location:\0"
   "p3p:\0"
   "vary:\0"
   "etag:\0"
   "accept-ranges:\0"
   "tcn:\0"
   "mime-version:\0"
   "www-authenticate:\0"
#ifdef _DEBUG
   "server:\0"
   "cache-controller\0"
   "age\0"
   "reply-to\0"
   "via\0"
   "composed-by\0"
   "keep-alive\0"
   "content-md5\0"
   "pics-label\0"
   "allow\0"   //get|post
   "microsoftofficewebserver\0"
#endif
/*
   Age
   Proxy-Authenticate
   Retry-After
   WWW-Authenticate
*/
   ;

enum{
   KW_HTTP_RESPONSE,
   KW_HTTP_DATE,
   KW_HTTP_LAST_MODIFY,
   KW_HTTP_CONTENT_TYPE,
   KW_HTTP_CONTENT_TYPE1,
   KW_HTTP_CONTENT_LENGTH,
   KW_HTTP_TRANSFER_ENCODING,
   KW_HTTP_CONNECTION,
   KW_HTTP_CONECTION,
   KW_HTTP_CACHE_CONTROL,
   KW_HTTP_PRAGMA,
   KW_HTTP_TRAILER,
   KW_HTTP_UPGRADE,
   KW_HTTP_WARNING,
   KW_HTTP_LOCATION,
   KW_HTTP_SET_COOKIE,
   KW_HTTP_EXPIRES,
   KW_HTTP_CONTENT_LANGUAGE,
   KW_HTTP_CONTENT_ENCODING,
   KW_HTTP_CONTENT_LOCATION,
   KW_HTTP_P3P,
   KW_HTTP_VARY,
   KW_HTTP_ETAG,
   KW_HTTP_ACCEPT_RANGES,
   KW_HTTP_TCN,
   KW_HTTP_MIME_VERSION,
   KW_HTTP_WWW_AUTHENTICATE,
#ifdef _DEBUG
   KW_HTTP_SERVER,
   KW_HTTP_CACHE_CONTROLLER,
   KW_HTTP_AGE,
   KW_HTTP_REPLY_TO,
   KW_HTTP_VIA,
   KW_HTTP_COMPOSED_BY,
   KW_HTTP_KEEP_ALIVE,
   KW_HTTP_CONTENT_MD5,
   KW_HTTP_PICS_LABEL,
   KW_HTTP_ALLOW,
   KW_HTTP_MSOWEBSERVER,
#endif
};

//----------------------------

bool C_application_http::S_http_header::Parse(const char *cp){

   Cstr_c val;
   while(*cp){
      int ki = text_utils::FindKeyword(cp, http_keywords);
      text_utils::SkipWS(cp);

      switch(ki){
      case KW_HTTP_RESPONSE:
         {
            int lo, hi;
            if(text_utils::ScanDecimalNumber(cp, hi) && *cp++=='.' && text_utils::ScanDecimalNumber(cp, lo)){
               http_ver = (hi<<8) | lo;
               text_utils::ScanDecimalNumber(cp, response_code);
            }
         }
         break;
      case KW_HTTP_DATE:
         {
            /*
            S_date_time dt;
            if(ReadHTTPDateTime(cp, dt))
               date = dt.sort_value;
            else
               assert(0);
               */
         }
         break;

      case KW_HTTP_LAST_MODIFY:
         {
            S_date_time dt;
            if(ReadHTTPDateTime(cp, dt)){
               last_modify_date = dt.sort_value;
            }else
               assert(0);
         }
         break;

      case KW_HTTP_CONTENT_TYPE:
      case KW_HTTP_CONTENT_TYPE1:
         content.ParseWithCoding(cp, content_coding);
         break;
      case KW_HTTP_CONTENT_LENGTH:
         text_utils::ScanDecimalNumber(cp, (int&)content_length);
         break;
      case KW_HTTP_TRANSFER_ENCODING:
         if(!text_utils::ReadWord(cp, val, text_utils::specials_rfc_822))
            break;
         if(val=="chunked"){
            chunked = true;
         }else{
            assert(0);
         }
         break;

      case KW_HTTP_LOCATION:
         ReadPhrase(cp, location);
         break;

      case KW_HTTP_WWW_AUTHENTICATE:
         if(text_utils::CheckStringBegin(cp, "basic"))
            flags |= FLG_AUTH_BASIC;
         break;

      case KW_HTTP_CONNECTION:
      case KW_HTTP_CONECTION:
         if(!text_utils::ReadWord(cp, val, text_utils::specials_rfc_822))
            break;
         val.ToLower();
         //if(val=="keep-alive")
            //keep_alive = true;
         if(val=="close")
            connection_close = true;
#ifdef _DEBUG
         else
         if(val=="keep-alive" || val=="transfer-encoding");
         else
            assert(0);
#endif
         break;
#ifdef WEB_
      case KW_HTTP_SET_COOKIE:
         if(config.flags&config.CONF_USE_COOKIES){
                              //read cookie
            S_cookie cok;
                              //read cookie name and value
            if(ReadToken(cp, cok.name, "= \t")){
               text_utils::SkipWS(cp);
               if(*cp++=='='){
                  text_utils::SkipWS(cp);
                  ReadToken(cp, cok.value, "; \t");
                  {
                              //read attributes
                     cok.expire_date = 0;
                     Cstr_c domain, path;
                     while(true){
                        text_utils::SkipWS(cp);
                        if(*cp!=';')
                           break;
                        ++cp;
                        text_utils::SkipWS(cp);
                        if(!*cp)
                           break;
                        Cstr_c attr, val;
                        if(!ReadToken(cp, attr, "=")){
                           assert(0);
                           break;
                        }
                        text_utils::SkipWS(cp);
                        if(*cp!='='){
                           assert(0);
                           break;
                        }
                        ++cp;
                        if(!ReadToken(cp, val, ";")){
                           assert(0);
                           break;
                        }
                        attr.ToLower();

                        if(attr=="expires"){
                              //if 'expires' is not set explicitly, then it defaults to end-of-session
                           S_date_time dt;
                           const char *cp = val;
                           if(!ReadHTTPDateTime(cp, dt)){
                              assert(0);
                              break;
                           }
                           cok.expire_date = dt.sort_value;
                        }else
                        if(attr=="path"){
                           path = val;
                        }else
                        if(attr=="domain"){
                              //detect invalidly-specified domain without leading dot
                           if(val[0]!='.')
                              domain<<'.';
                           domain<<val;
                        }
#ifdef _DEBUG
                        else
                        if(attr=="comment"){
                        }else
                        if(attr=="version"){
                        }else
                        if(attr=="max-age"){
                           //assert(0);
                        }else
                           assert(0);
#endif
                     }
                     if(!domain.Length()){
                              //if Domain is not set explicitly, then it defaults to the full domain of the document creating the cookie
                        int dot = ldr.curr_connected_domain.Find('.');
                        if(dot!=-1)
                           domain = ldr.curr_connected_domain.Right(ldr.curr_connected_domain.Length()-dot);
                        else
                           domain = ldr.curr_connected_domain;
                     }
                     if(!path.Length()){
                              //if 'path' is not set explicitly, then it defaults to the URL path of the document creating the cookie
                        path = ldr.curr_file;
                        int slash = path.FindReverse('/');
                        if(slash!=-1)
                           path = path.Left(slash+1);
                     }
                     //assert(cok.expire_date!=0);
                              //check if security rules are satisfied, if not, reject cookie
                     bool ok = true;

                     if(domain[0]!='.' || domain.Length()<2 || !path.Length() || path[0]!='/'){
                              //domain must begin with dot
                        ok = false;
                     }else
                     if(domain.Mid(1, domain.Length()-2).FindReverse('.') == -1){
                              //domain must contain embedded dot
                        ok = false;
                     }else
                     if(ldr.curr_file.Length() < path.Length() || ldr.curr_file.Left(path.Length())!=path){
                              //the value for the Path attribute is not a prefix of the request-URI
                        ok = false;
                     }else
                     if(ldr.curr_connected_domain.Length() < domain.Length() || ldr.curr_connected_domain.Right(domain.Length())!=domain){
                              //the value for the request-host does not domain-match the Domain attribute
                        ok = false;
                     }else
                     if(ldr.curr_connected_domain.Find('.') < (ldr.curr_connected_domain.Length()-domain.Length())){
                              //the request-host has the form HD, where D is the value of the Domain attribute, and H is a string that contains one or more dots.
                        ok = false;
                     }
                     if(ok){
                              //cookie accepted
                        cok.domain_path<<domain <<path;
                        reinterpret_cast<C_web_client*>(this)->InsertUpdateCookie(cok);
                     }else{
                        //assert(0);
                     }
                  //}else{
                     //assert(0);
                  }
               }else{
                  assert(0);
               }
            }else{
               assert(0);
            }
         }
         break;

      case KW_HTTP_CACHE_CONTROL:
         while(*cp){
            text_utils::SkipWS(cp);
            Cstr_c attr, val;
            ReadToken(cp, attr, " =,");
            text_utils::SkipWS(cp);
            if(*cp=='='){
               ++cp;
               text_utils::SkipWS(cp);
               ReadToken(cp, val, " =,");
            }
            attr.ToLower();
            if(attr=="public"){
                              //(rfc2616) Indicates that the response MAY be cached by any cache, even if it would normally be non-cacheable
                              // or cacheable only within a non-shared cache.

                              //action: ignore
            }else
            if(attr=="private"){
                              //(rfc2616) Indicates that all or part of the response message is intended for a single user and MUST NOT be cached
                              // by a shared cache. This allows an origin server to state that the specified parts of the response are
                              // intended for only one user and are not a valid response for requests by other users. A private (non-shared)
                              // cache MAY cache the response.
                              //Note: This usage of the word private only controls where the response may be cached, and cannot ensure the
                              // privacy of the message content.

                              //action: ignore - mobile device is considered private, single-user computer
            }else
            if(attr=="no-cache"){
                              //If the no-cache directive does not specify a field-name, then a cache MUST NOT use the response to satisfy
                              // a subsequent request without successful revalidation with the origin server. This allows an origin server to
                              // prevent caching even by caches that have been configured to return stale responses to client requests.
                              //If the no-cache directive does specify one or more field-names, then a cache MAY use the response to satisfy
                              // a subsequent request, subject to any other restrictions on caching. However, the specified field-name(s)
                              // MUST NOT be sent in the response to a subsequent request without successful revalidation with the origin server.
                              //This allows an origin server to prevent the re-use of certain header fields in a response, while still allowing
                              // caching of the rest of the response.
                              //Note: Most HTTP/1.0 caches will not recognize or obey this directive.
               if(val.Length()){
                  assert(0);
               }else{
                              //action: set flag, which causes no storing in cache
                  flags |= FLG_CACHE_NO_CACHE;
               }
            }else
            if(attr=="no-store"){
                              /*
                              The purpose of the no-store directive is to prevent the inadvertent release or retention of sensitive information
                              (for example, on backup tapes). The no-store directive applies to the entire message.
                              A cache MUST NOT store any part of either this response or the request that elicited it. This directive applies to
                              both non-shared and shared caches. "MUST NOT store" in this context means that the cache MUST NOT intentionally
                              store the information in non-volatile storage, and MUST make a best-effort attempt to remove the information from
                              volatile storage as promptly as possible after forwarding it.

                              Even when this directive is associated with a response, users might explicitly store such a response outside of
                              the caching system (e.g., with a "Save As" dialog). History buffers MAY store such responses as part of their
                              normal operation.

                              The purpose of this directive is to meet the stated requirements of certain users and service authors who are
                              concerned about accidental releases of information via unanticipated accesses to cache data structures.
                              While the use of this directive might improve privacy in some cases, we caution that it is NOT in any way a reliable
                              or sufficient mechanism for ensuring privacy. In particular, malicious or compromised caches might not recognize or
                              obey this directive, and communications networks might be vulnerable to eavesdropping.
                              */

                              //action: ignore
            }else
            if(attr=="must-revalidate"){
                              /*
                              Because a cache MAY be configured to ignore a server's specified
                              expiration time, and because a client request MAY include a max-
                              stale directive (which has a similar effect), the protocol also
                              includes a mechanism for the origin server to require revalidation
                              of a cache entry on any subsequent use. When the must-revalidate
                              directive is present in a response received by a cache, that cache
                              MUST NOT use the entry after it becomes stale to respond to a

                              subsequent request without first revalidating it with the origin
                              server. (I.e., the cache MUST do an end-to-end revalidation every
                              time, if, based solely on the origin server's Expires or max-age
                              value, the cached response is stale.)

                              The must-revalidate directive is necessary to support reliable
                              operation for certain protocol features. In all circumstances an
                              HTTP/1.1 cache MUST obey the must-revalidate directive; in
                              particular, if the cache cannot reach the origin server for any
                              reason, it MUST generate a 504 (Gateway Timeout) response.

                              Servers SHOULD send the must-revalidate directive if and only if
                              failure to revalidate a request on the entity could result in
                              incorrect operation, such as a silently unexecuted financial
                              transaction. Recipients MUST NOT take any automated action that
                              violates this directive, and MUST NOT automatically provide an
                              unvalidated copy of the entity if revalidation fails.

                              Although this is not recommended, user agents operating under
                              severe connectivity constraints MAY violate this directive but, if
                              so, MUST explicitly warn the user that an unvalidated response has
                              been provided. The warning MUST be provided on each unvalidated
                              access, and SHOULD require explicit user confirmation.
                              */

                              //action: set flags, which force re-validation, even if cached
               flags |= FLG_CACHE_MUST_REVALIDATE;
            }else
            if(attr=="max-age"){
                              //Indicates that the client is willing to accept a response whose age is no greater than the specified time in
                              // seconds. Unless max-stale directive is also included, the client is not willing to accept a stale response.
               int delta;
               const char *cp = val;
               if(text_utils::ScanDecimalNumber(cp, delta)){
                  expire_date = C_web_client::GetLocalGMTTime() + delta;
               }else
                  assert(0);
            }
#ifdef _DEBUG
            else
            if(attr=="proxy-revalidate"){
            }else
            if(attr=="no-cache"){
            }else
            if(attr=="post-check" || attr=="pre-check" || attr=="no-transform"){
                              //cache extensions, ignore
            }else{
               //assert(0);
            }
#endif
            text_utils::SkipWS(cp);
            if(*cp==',')
               ++cp;
         }
         break;

      case KW_HTTP_PRAGMA:
         {
            text_utils::SkipWS(cp);
            Cstr_c attr;
            ReadToken(cp, attr, " =,");
            attr.ToLower();
            if(attr=="no-cache"){
                              //same as Cache-control: no-cache
               flags |= FLG_CACHE_NO_CACHE;
            }
#ifdef _DEBUG
            else
               assert(0);
#endif
         }
         break;

      case KW_HTTP_EXPIRES:
         {
                              /*
                              The Expires entity-header field gives the date/time after which the response is considered stale. A stale cache
                              entry may not normally be returned by a cache unless it is first validated with the origin server (or with an
                              intermediate cache that has a fresh copy of the entity).

                              The presence of an Expires field does not imply that the original resource will change or cease to exist at,
                              before, or after that time.

                                 Note: if a response includes a Cache-Control field with the max-age directive (see section 14.9.3),
                                 that directive overrides the Expires field.

                              HTTP/1.1 clients and caches MUST treat other invalid date formats, especially including the value "0",
                              as in the past (i.e., "already expired").

                              To mark a response as "already expired," an origin server sends an Expires date that is equal to the
                              Date header value. (See the rules for expiration calculations in section 13.2.4.)

                              To mark a response as "never expires," an origin server sends an Expires date approximately one year from
                              the time the response is sent. HTTP/1.1 servers SHOULD NOT send Expires dates more than one year in the future.

                              The presence of an Expires header field with a date value of some time in the future on a response that otherwise
                              would by default be non-cacheable indicates that the response is cacheable, unless indicated otherwise by a
                              Cache-Control header field.
                              */
            if(!expire_date){
               S_date_time dt;
               if(ReadHTTPDateTime(cp, dt)){
                  expire_date = dt.sort_value;
               }else{
                  if(text_utils::ScanDecimalNumber(cp, (int&)expire_date)){
                     if(!expire_date)
                        expire_date = 1;
                  }else{
                     assert(0);
                  }
               }
            }
         }
         break;

      case KW_HTTP_CONTENT_LANGUAGE:
         cp = cp;
         break;

      case KW_HTTP_P3P:
      case KW_HTTP_TCN:
      case KW_HTTP_MIME_VERSION:
         break;

      case KW_HTTP_CONTENT_ENCODING:
         {
            Cstr_c cod;
            if(ReadToken(cp, cod, ", \t")){
               cod.ToLower();
               if(cod=="deflate"){
                  content_encoding = ENCODING_DEFLATE;
               }else
               if(cod=="gzip" || cod=="x-gzip"){
                  content_encoding = ENCODING_GZIP;
               }else
               if(cod=="compress" || cod=="x-compress"){
                  content_encoding = ENCODING_COMPRESS;
               }else
                  assert(0);
#ifdef _DEBUG
                              //make sure only one encoding is present
               text_utils::SkipWS(cp);
               if(*cp==','){
                  assert(0);
               }
#endif
            }
         }
         break;

      case KW_HTTP_CONTENT_LOCATION:
         break;

      case KW_HTTP_VARY:
         break;

      case KW_HTTP_ACCEPT_RANGES:
         break;

      case KW_HTTP_ETAG:
         ReadToken(cp, etag, " \t");
         break;

#ifdef _DEBUG
      case KW_HTTP_CACHE_CONTROLLER:
      case KW_HTTP_AGE:
      case KW_HTTP_REPLY_TO:
      case KW_HTTP_SERVER:
      case KW_HTTP_VIA:
      case KW_HTTP_COMPOSED_BY:
      case KW_HTTP_KEEP_ALIVE:
      case KW_HTTP_CONTENT_MD5:
      case KW_HTTP_PICS_LABEL:
      case KW_HTTP_ALLOW:
      case KW_HTTP_MSOWEBSERVER:
         break;
#endif

      default:
         if(ToLower(*cp)=='x' && cp[1]=='-'){
         }else{
            //assert(0);
         }
#endif//WEB
      }
                              //go to next line
      while(*cp){
         if(*cp++=='\n')
            break;
      }
   }
   return true;
}

//----------------------------

void C_data_saving::CancelOutstanding(){
                           //automatically delete file if storing was not finished normally
   if(IsStarted()){
      fl.Close();
      C_file::DeleteFile(filename);
      filename.Clear();
   }
}

//----------------------------

bool C_data_saving::Start(const Cstr_w &full_name){

   CancelOutstanding();
   if(fl.Open(full_name, C_file::FILE_WRITE)){
      filename = full_name;
      return true;
   }
   return false;
}

//----------------------------

void C_application_http::C_http_data_loader::Reset(){

   C_http_data_loader l;
   *this = l;
}

//----------------------------
// From URL (with http:// prefix) extract domain name.
static void GetDomain(const Cstr_c &url, Cstr_c &domain){

   const char *cp = url;
   if(text_utils::CheckStringBegin(cp, text_utils::HTTP_PREFIX) || text_utils::CheckStringBegin(cp, text_utils::HTTPS_PREFIX)){
      domain = url;
      dword i;
      for(i=0; i<domain.Length(); i++){
         if(domain[i]=='?'){
            if(i<domain.Length()-1)
               //domain.At(i+1) = 0;
               domain.At(i) = 0;
            domain = (const char*)domain;
            break;
         }
      }
      i = domain.Length();
      if(i && domain[i-1]!='/'){
         while(--i > 6){
            if(domain[i]=='/'){
               domain.At(i+1) = 0;
               domain = (const char*)domain;
               break;
            }
         }
         if(i==6)
            domain<<'/';
      }
                              //detect :xx (port number) at the end
      for(i=7; i<domain.Length(); i++){
         if(domain[i]==':'){
            const char *cp1 = domain+i+1;
            int port_i;
            if(text_utils::ScanDecimalNumber(cp1, port_i)){
               Cstr_c line_end = cp1;
               domain.At(i) = 0;
               domain = (const char*)domain;
               domain<<line_end;
               break;
            }
         }
      }
   }else
      assert(0);
}

//----------------------------

bool C_application_http::Loader_ConnectTo(C_http_data_loader &ldr, const Cstr_c &url, const wchar *log_file, void *socket_notify, int override_timeout){

   assert(temp_path);
   ldr.phase = ldr.NOT_CONNECTED;
   const char *cp = url;
   Cstr_c curr_domain;
   bool use_ssl = false;
   if(text_utils::CheckStringBegin(cp, text_utils::HTTP_PREFIX) || (use_ssl=true, text_utils::CheckStringBegin(cp, text_utils::HTTPS_PREFIX))){
                              //check if userinfo is present
      ldr.userinfo.Clear();
      int ui = url.Find('@');
      if(ui!=-1){
         Cstr_c url_plain = cp;
         int slash_i = url_plain.Find('/'), q_i = url_plain.Find('?');
         if((slash_i==-1 || slash_i>ui) && (q_i==-1 || q_i>ui)){
                              //@ is accepted only before first /, . or ? (not as part of parameters)
            ui -= cp - (const char*)url;
            ldr.userinfo.Allocate(cp, ui);
            cp += ui+1;
         }
      }
      if(text_utils::ReadWord(cp, curr_domain, "/\\:")){
         GetDomain(url, ldr.domain);

         word port = word(use_ssl ? C_socket::PORT_HTTP_SSL : C_socket::PORT_HTTP);
         if(*cp==':'){
                                 //skip port specification
            ++cp;
            int port_i;
            if(text_utils::ScanDecimalNumber(cp, port_i))
               port = word(port_i);
         }
         if(!*cp)
            cp = "/";
         Cstr_c &fname = ldr.curr_file;
         fname.Clear();
         fname<<cp;
                                 // convert special chars to %xx
         for(int i=fname.Length(); i--; ){
            char c = fname[i];
            switch(c){
            //case '\\': fname.At(i) = '/'; break;
            case ' ':
            case '\n':
               C_vector<char> buf;
               const char *cp1 = fname;
               buf.reserve(fname.Length()+3);
               buf.insert(buf.end(), cp1, cp1+i);
               char hex_s[] = "%00";
               word h = text_utils::MakeHexString(c);
               MemCpy(hex_s+1, &h, 2);
               buf.insert(buf.end(), hex_s, hex_s+3);
               buf.insert(buf.end(), cp1+i+1, cp1+fname.Length());
               buf.push_back(0);
               fname = buf.begin();
               break;
            }
         }
         CreateConnection();
         ldr.socket = CreateSocket(connection, socket_notify ? socket_notify : ldr.mod_socket_notify, use_ssl, false, log_file);
            /*
#ifndef _WIN32_WCE
            L"C:"
#endif
            L"\\ConnectLog.txt");
            */
         ldr.socket->Release();

         ldr.socket->Connect(curr_domain, port, override_timeout);
         ldr.phase = ldr.CONNECTING;
         ldr.curr_domain = curr_domain;
         ldr.prog_curr.pos = 0; ldr.prog_curr.total = 1;
         ldr.system_error_text.Clear();
                                 //reset header
         S_http_header h;
         ldr.hdr = h;
         return true;
      }
   }
   assert(0);
   return false;
}

//----------------------------

C_application_http::E_HTTP_LOADER C_application_http::TickDataLoader(C_http_data_loader &ldr, C_socket_notify::E_SOCKET_EVENT ev, bool &redraw){

   if(ldr.phase==ldr.CLOSED)
      return LDR_CLOSED;
   switch(ev){
   case C_socket_notify::SOCKET_CONNECTED:
      {
                                 //send GET request
         Cstr_c s;
         C_http_data_loader::E_METHOD method = ldr.http_method;
         if(method==ldr.METHOD_AUTO){
                              //auto-detect method
            method = (ldr.post_data.Length() || ldr.binary_post_data) ? C_http_data_loader::METHOD_POST : C_http_data_loader::METHOD_GET;
         }
         static const char *const methods[] = { "GET", "POST", "PUT", "DELETE" };
         s <<methods[method-1];
         s<<' ' <<ldr.curr_file <<" HTTP/1.1\r\n";
         s.AppendFormat(
            "host: %\r\n"
            "Connection: close\r\n"
            )
            <<ldr.curr_domain
            ;
         if(ldr.userinfo.Length())
            s.AppendFormat("Authorization: Basic %\r\n") <<text_utils::StringToBase64(ldr.userinfo);
         if(ldr.add_header_fields.Length())
            s<<ldr.add_header_fields <<"\r\n";
         if(ldr.post_data.Length() || ldr.binary_post_data){
            s<<"Content-Type: ";
            if(ldr.content_type.Length())
               s<<ldr.content_type;
            else
               s<<"application/x-www-form-urlencoded";
            s<<"\r\n";
            int len = ldr.post_data.Length();
            if(!len){
               len = ldr.binary_post_data->GetLength();
                              //prepare progress
               ldr.prog_curr.total = len;
               ldr.prog_curr.pos = 0;
            }
            s.AppendFormat("Content-Length: %\r\n") <<len;
         }
         s<<"\r\n";
         if(ldr.post_data.Length()){
            s<<ldr.post_data;
            ldr.post_data.Clear();
         }
         ldr.socket->SendString(s);
         //ldr.phase = C_http_data_loader::SEND_REQUEST;
         ldr.phase = ldr.binary_post_data ? C_http_data_loader::POST_DATA : C_http_data_loader::WAIT_RESPONSE_HDR;
      }
      break;

   case C_socket_notify::SOCKET_DATA_SENT:
      if(ldr.phase==C_http_data_loader::POST_DATA){
         assert(ldr.binary_post_data);
         dword max_len = Min(4096, ldr.prog_curr.total-ldr.prog_curr.pos);
         byte *buf = new(true) byte[max_len];
         dword read_num = ldr.binary_post_data->ReadData(buf, max_len);
         ldr.prog_curr.pos += read_num;
         ldr.socket->SendData(buf, read_num);
         delete[] buf;
         if(ldr.prog_curr.pos==ldr.prog_curr.total){
                              //sent all data
            ldr.binary_post_data = NULL;
            ldr.phase = C_http_data_loader::WAIT_RESPONSE_HDR;
         }
      }
      break;

   case C_socket_notify::SOCKET_DATA_RECEIVED:
                              //process all available data
      for(bool loop=true; loop; ){
         C_socket::t_buffer line;
         switch(ldr.phase){
         case C_http_data_loader::READ_DATA:
         case C_http_data_loader::READ_DATA_CHUNK:
            if(ldr.socket->GetData(line, 1, ldr.phase==C_http_data_loader::READ_DATA ? ldr.length_to_read : ldr.chunk_size)){
               const char *cp = line.Begin();

               dword sz = line.Size();
               if(ldr.data_saving.fl.Write(cp, sz)){
                              //write error
                  ldr.socket = NULL;
                  redraw = true;
                  return LDR_FAILED;
               }
               ldr.prog_curr.pos += sz;
                           //for non-size-specified, chunked downloads, loop progress
               if(!ldr.hdr.content_length && ldr.hdr.chunked && ldr.prog_curr.pos >= ldr.prog_curr.total)
                  ldr.prog_curr.pos = 0;
               if(ldr.phase==C_http_data_loader::READ_DATA){
                  if(ldr.length_to_read)
                  if(!ldr.hdr.connection_close)
                  {
                     ldr.length_to_read -= sz;
                     if(!ldr.length_to_read){
                        ldr.socket = NULL;
                        if(ldr.data_saving.fl.WriteFlush()){
                           return LDR_FAILED;
                        }
                        ldr.data_saving.fl.Close();
                        redraw = true;
                        return ldr.http_code>=400 ? LDR_NOT_FOUND : LDR_DONE;
                     }
                  }//else
                     //assert(ldr.hdr.connection_close);
               }else{
                  ldr.chunk_size -= sz;
                  assert(ldr.chunk_size >= 0);
                  if(ldr.chunk_size <= 0){
                        //start next chunk
                     ldr.phase = C_http_data_loader::READ_DATA_CHUNK_END;
                     redraw = true;
                     return TickDataLoader(ldr, ev, redraw);
                  }
               }
                        //make screen to redraw (due to new image, or progress bar)
               redraw = true;
            }else
               loop = false;
            break;

         default:
            if(!ldr.socket->GetLine(line)){
               loop = false;
               break;
            }
            const char *cp = line.Begin();
            switch(ldr.phase){
            case C_http_data_loader::WAIT_RESPONSE_HDR:
                              //support HTTP 1.0 or 1.1
               if(text_utils::CheckStringBegin(cp, "http/1.")){
                  int http_ver = *cp++ - '0';
                  if((http_ver==0 || http_ver==1) && text_utils::ScanDecimalNumber(cp, ldr.http_code)){
                        //status code:
                        // 1xx: Informational - Request received, continuing process
                        // 2xx: Success - The action was successfully received, understood, and accepted
                        // 3xx: Redirection - Further action must be taken in order to complete the request
                        // 4xx: Client Error - The request contains bad syntax or cannot be fulfilled
                        // 5xx: Server Error - The server failed to fulfill an apparently valid request
                     int cls = ldr.http_code/100;
                     switch(cls){
                     case 2: //success
                        ldr.phase = C_http_data_loader::READ_HDR;
                        break;
                     case 3: //redirected
                        ldr.phase = C_http_data_loader::READ_HDR_REDIRECT;
                        break;
                     case 4: //not found
                     case 5: //server error
                        ldr.phase = C_http_data_loader::READ_HDR;
                        break;
                     default:
                        ldr.socket = NULL;
                        redraw = true;
                        return LDR_NOT_FOUND;
                     }
                  }else{
                     assert(0);
                     redraw = true;
                     return LDR_FAILED;
                  }
               }else{
                  assert(0);
                  redraw = true;
                  return LDR_FAILED;
               }
               break;

            case C_http_data_loader::READ_HDR:
            case C_http_data_loader::READ_HDR_REDIRECT:
               if(*cp){
                              //parse header line
                  ldr.hdr.Parse(cp);
               }else{
                  if(ldr.phase==C_http_data_loader::READ_HDR_REDIRECT){
                              //process redirection
                     ldr.socket = NULL;
                     const char *cp1 = ldr.hdr.location;
                     if(!text_utils::CheckStringBegin(cp1, text_utils::HTTP_PREFIX) && !text_utils::CheckStringBegin(cp1, text_utils::HTTPS_PREFIX))
                        text_utils::MakeFullUrlName(ldr.domain, ldr.hdr.location);
                     GetDomain(ldr.hdr.location, ldr.domain);
                     Loader_ConnectTo(ldr, ldr.hdr.location);
                     loop = false;
                     break;
                  }
                              //last line of header, begin downloading data
                  Cstr_w full_name;
                  if(ldr.dst_filename.Length() && ldr.dst_filename[ldr.dst_filename.Length()-1]!='\\'){
                              //if dest specified, we must be downloading exactly one file
                     full_name = ldr.dst_filename;
                  }else{
                              //get extension of proposed filename
                     Cstr_c cfc = ldr.curr_file;
                     Cstr_w cfw; cfw.Copy(cfc);
                     const wchar *ext = text_utils::GetExtension(cfw);
                     if(!ext){
                        cfw.Copy(ldr.hdr.content.subtype);
                        ext = cfw;
                     }else{
                              //make sure ext is valid
                        for(const wchar *cp1=ext; *cp1; ){
                           wchar c = ToLower(*cp1++);
                           if(!(c>='a' && c<='z')){
                              cfw.Copy(ldr.hdr.content.subtype);
                              ext = cfw;
                              break;
                           }
                        }
                     }
                     if(!*ext)
                        ext = L"att";
                     Cstr_w filename;
                     {
                        int i;
                        int dot_pos = -1;
                        for(i=ldr.curr_file.Length(); i--; ){
                           char c = ldr.curr_file[i];
                           if(c=='.')
                              dot_pos = i;
                           if(c=='/')
                              break;
                        }
                        filename.Copy(&cfc[i+1]);
                        if(dot_pos!=-1){
                           filename.At(dot_pos-i-1) = 0;
                           filename = (const wchar*)filename;
                        }
                     }
                     for(int i=filename.Length(); i--; ){
                        wchar c = filename[i];
                        if(text_utils::IsAlNum(c) || c=='_')
                           continue;
                        filename.Clear();
                        break;
                     }
                     const wchar *base_path;
                     if(ldr.dst_filename.Length())
                        base_path = ldr.dst_filename;
                     else
                        base_path = temp_path;
                     file_utils::MakeUniqueFileName(base_path, filename, ext);
                     full_name<<base_path <<filename;
                  }
                  C_file::MakeSurePathExists(full_name);
                  if(ldr.data_saving.Start(full_name)){
                     ldr.phase = ldr.hdr.chunked ? ldr.READ_DATA_CHUNK_BEGIN : ldr.READ_DATA;
                     ldr.chunk_size = 0;
                     ldr.prog_curr.total = ldr.hdr.content_length ? ldr.hdr.content_length : 20000;
                     ldr.length_to_read = ldr.hdr.content_length;
                  }else{
                     assert(0);
                     redraw = true;
                     return LDR_FAILED;
                  }
               }
               break;

            case C_http_data_loader::READ_DATA_CHUNK_BEGIN:
               if(!*cp){
                  loop = false;
                  break;
               }
               if(!text_utils::ScanHexNumber(cp, ldr.chunk_size)){
                  redraw = true;
                  return LDR_FAILED;
               }
               text_utils::SkipWS(cp);
               if(*cp==';'){
                              //read chunk extension
                  assert(0);
               }
               if(ldr.chunk_size){
                  //ldr.prog_curr.total = ldr.chunk_size;
                  ldr.phase = C_http_data_loader::READ_DATA_CHUNK;
               }else{
                  redraw = true;
                              //last chunk, store
                  ldr.socket = NULL;
                  if(ldr.data_saving.fl.WriteFlush()){
                     return LDR_FAILED;
                  }
                  ldr.data_saving.fl.Close();
                  return ldr.http_code>=400 ? LDR_NOT_FOUND : LDR_DONE;
               }
               break;

            case C_http_data_loader::READ_DATA_CHUNK_END:
               ldr.phase = C_http_data_loader::READ_DATA_CHUNK_BEGIN;
               break;

            default:
               assert(0);
            }
         }
      }
      break;

   case C_socket_notify::SOCKET_FINISHED:
                              //connection finished (closed by server)
      assert(ldr.data_saving.filename.Length());
      ldr.data_saving.fl.Close();
      redraw = true;
      return ldr.http_code>=400 ? LDR_NOT_FOUND : LDR_DONE;

   case C_socket_notify::SOCKET_ERROR:
      ldr.system_error_text = ldr.socket->GetSystemErrorText();
      redraw = true;
      return LDR_FAILED;
   }
   return LDR_PROGRESS;
}

//----------------------------

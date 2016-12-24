//
//  raytwitteroauth.cpp
//
//  Created by leico on 2016/12/23.
//
//

#include <iostream>
#include <string>
#include <TwitterOAuth.hpp>
#include <picojson.h>

#include "ext.h"
#include "ext_obex.h"

#define PICOJSON_USE_INT64

typedef struct _rayTwitterOAuth{
  t_object  object;
  void     *out;


  void *queue;


  t_symbol *method;
  t_symbol *url;
  t_symbol *consumer_key;
  t_symbol *consumer_secret;
  t_symbol *access_token;
  t_symbol *token_secret;

  int connect;
  TwitterOAuth *twitter;
  std :: string *tweetdata;

  int count;
} rayTwitterOAuth;

void* newObject (t_symbol *s, long argc, t_atom *argv);
void  freeObject(rayTwitterOAuth *x);
void  assistNavi(rayTwitterOAuth *x, void *b, long m, long a, char *s);

void curlUpdate(rayTwitterOAuth *x);

void Connect(rayTwitterOAuth *x, long n);

size_t responseCallback(char *data, size_t size, size_t nmemb, void *userdata);
int    progressCallback(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

void *class_rayTwitterOAuth;

void ext_main(void *r){

  t_class *c;

  c = class_new("ray.twitteroauth", (method)newObject, (method)freeObject, (long)sizeof(rayTwitterOAuth), 0L, A_GIMME, 0);

  class_addmethod(c, (method)assistNavi, "assist", A_CANT, 0);
  class_addmethod(c, (method)Connect,    "int",    A_LONG, 0);




  CLASS_ATTR_SYM (c, "method",          ATTR_FLAGS_NONE, rayTwitterOAuth, method);
  CLASS_ATTR_SYM (c, "url",             ATTR_FLAGS_NONE, rayTwitterOAuth, url);
  CLASS_ATTR_SYM (c, "consumer_key",    ATTR_FLAGS_NONE, rayTwitterOAuth, consumer_key);
  CLASS_ATTR_SYM (c, "consumer_secret", ATTR_FLAGS_NONE, rayTwitterOAuth, consumer_secret);
  CLASS_ATTR_SYM (c, "access_token",    ATTR_FLAGS_NONE, rayTwitterOAuth, access_token);
  CLASS_ATTR_SYM (c, "token_secret",    ATTR_FLAGS_NONE, rayTwitterOAuth, token_secret);

  CLASS_ATTR_SAVE(c, "method",          ATTR_FLAGS_NONE);
  CLASS_ATTR_SAVE(c, "url",             ATTR_FLAGS_NONE);
  CLASS_ATTR_SAVE(c, "consumer_key",    ATTR_FLAGS_NONE);
  CLASS_ATTR_SAVE(c, "consumer_secret", ATTR_FLAGS_NONE);
  CLASS_ATTR_SAVE(c, "access_token",    ATTR_FLAGS_NONE);
  CLASS_ATTR_SAVE(c, "token_secret",    ATTR_FLAGS_NONE);

  CLASS_ATTR_LABEL(c, "method",          ATTR_FLAGS_NONE, "REST method");
  CLASS_ATTR_LABEL(c, "url",             ATTR_FLAGS_NONE, "URL");
  CLASS_ATTR_LABEL(c, "consumer_key",    ATTR_FLAGS_NONE, "Consumer Key");
  CLASS_ATTR_LABEL(c, "consumer_secret", ATTR_FLAGS_NONE, "Consumer Secret");
  CLASS_ATTR_LABEL(c, "access_token",    ATTR_FLAGS_NONE, "Access Token");
  CLASS_ATTR_LABEL(c, "token_secret",    ATTR_FLAGS_NONE, "Access Token Secret");

  CLASS_ATTR_CATEGORY(c, "method",          ATTR_FLAGS_NONE, "TwitterOAuth");
  CLASS_ATTR_CATEGORY(c, "url",             ATTR_FLAGS_NONE, "TwitterOAuth");
  CLASS_ATTR_CATEGORY(c, "consumer_key",    ATTR_FLAGS_NONE, "TwitterOAuth");
  CLASS_ATTR_CATEGORY(c, "consumer_secret", ATTR_FLAGS_NONE, "TwitterOAuth");
  CLASS_ATTR_CATEGORY(c, "access_token",    ATTR_FLAGS_NONE, "TwitterOAuth");
  CLASS_ATTR_CATEGORY(c, "token_secret",    ATTR_FLAGS_NONE, "TwitterOAuth");

  CLASS_ATTR_ORDER(c, "method",          ATTR_FLAGS_NONE, "1");
  CLASS_ATTR_ORDER(c, "url",             ATTR_FLAGS_NONE, "2");
  CLASS_ATTR_ORDER(c, "consumer_key",    ATTR_FLAGS_NONE, "3");
  CLASS_ATTR_ORDER(c, "consumer_secret", ATTR_FLAGS_NONE, "4");
  CLASS_ATTR_ORDER(c, "access_token",    ATTR_FLAGS_NONE, "5");
  CLASS_ATTR_ORDER(c, "token_secret",    ATTR_FLAGS_NONE, "6");

  class_register(CLASS_BOX, c);
  class_rayTwitterOAuth = c;
}


void* newObject (t_symbol *s, long argc, t_atom *argv){

  rayTwitterOAuth *x = NULL;

  if( (x = (rayTwitterOAuth *)object_alloc((t_class *)class_rayTwitterOAuth)) ){

    object_post((t_object *)x, "%s object instance created", s -> s_name);

    x -> out       = outlet_new(x, NULL);
    x -> connect   = 0;

    x -> queue = qelem_new((t_object *)x, (method)curlUpdate);
    
    x -> method          = gensym("");
    x -> url             = gensym("");
    x -> consumer_key    = gensym("");
    x -> consumer_secret = gensym("");
    x -> access_token    = gensym("");
    x -> token_secret    = gensym("");

    x -> twitter   = new TwitterOAuth();
    x -> tweetdata = NULL;
    x -> count     = 0;
  }

  return x;

}
void freeObject(rayTwitterOAuth *x){

  Connect(x, 0);

  qelem_free(x -> queue);
  delete x -> twitter;

}

void assistNavi(rayTwitterOAuth *x, void *b, long m, long a, char *s){

  if (m == ASSIST_INLET) { // inlet
    sprintf(s, "I am inlet %ld", a);
  }
  else {	// outlet
    sprintf(s, "I am outlet %ld", a);
  }

}

void curlUpdate(rayTwitterOAuth *x){

  x -> twitter -> CurlConnectCount();
  qelem_set(x -> queue);
}




void Connect(rayTwitterOAuth *x, long n){

  object_post((t_object *)x, "%d", n);

  n = (n != 0) ? 1 : 0;
  
  if(n == x -> connect) return;

  if(n == 1){

    char *check = x -> method -> s_name;

    if( strlen(check) == 0 ){
      object_error((t_object *)x, "REST method undefined!");
      return;
    }

    check = x -> url -> s_name;
    if( strlen(check) == 0 ){
      object_error((t_object *)x, "URL undefined!");
      return;
    }

    check = x -> consumer_key -> s_name;
    if( strlen(check) == 0 ){
      object_error((t_object *)x, "consumer key undefined!");
      return;
    }

     check = x -> consumer_secret -> s_name;
    if( strlen(check) == 0 ){
      object_error((t_object *)x, "consumer secret undefined!");
      return;
    }

    check = x -> access_token -> s_name;
    if( strlen(check) == 0 ){
      object_error((t_object *)x, "access token undefined!");
      return;
    }

     check = x -> token_secret -> s_name;
    if( strlen(check) == 0 ){
      object_error((t_object *)x, "token secret key undefined!");
      return;
    }

    x -> tweetdata = new std :: string();
    x -> tweetdata -> resize(65535);

    x -> twitter -> RESTMethod         ( x -> method          -> s_name );
    x -> twitter -> URL                ( x -> url             -> s_name );
    x -> twitter -> ConsumerKey        ( x -> consumer_key    -> s_name );
    x -> twitter -> ConsumerSecret     ( x -> consumer_secret -> s_name );
    x -> twitter -> AccessToken        ( x -> access_token    -> s_name );
    x -> twitter -> AccessTokenSecret  ( x -> token_secret    -> s_name );

    x -> twitter -> ResponseCallback(responseCallback);
    x -> twitter -> ProgressCallback(progressCallback);

    x -> twitter -> PassedResponseCallbackData(reinterpret_cast<void *>( x ));
    x -> twitter -> PassedProgressCallbackData(reinterpret_cast<void *>( x ));

    //thread start
    x -> twitter -> StartSendRequest();

    qelem_set(x -> queue);
  }
  else{

    qelem_unset(x -> queue);

    std :: vector<std :: string> msgs;
    x -> twitter -> CurlReadMsgs(msgs);

    for(int i = 0 ; i < msgs.size() ; ++ i)
      post(msgs.at(i).c_str());

    x -> twitter -> CurlDisconnect();
    delete x -> tweetdata;
  }

  x -> connect = n;
  return;
}


size_t responseCallback(char *data, size_t size, size_t nmemb, void *userdata){

  size_t realsize = size * nmemb;
  const std :: string ENDMARK("\r\n");

  //passed userdata (responseData pointer)
  rayTwitterOAuth *x = reinterpret_cast<rayTwitterOAuth *>(userdata);

  //get new response
  std :: string  str(data, realsize);

  //if only endmark, end function
  if(str == ENDMARK)
    return realsize;
 
  //joint new data
   *(x -> tweetdata) += str;

  //if not found end mark, its a piece of tweet data
  if(x -> tweetdata -> find(ENDMARK) == std :: string :: npos)
    return realsize;


  picojson :: value  json;
  std      :: string err("");

  picojson :: parse(json, x -> tweetdata -> begin(), x -> tweetdata -> end(), &err);

  //end when json parse error
  if(! err.empty()){
    object_warn((t_object *)x, "json parse error: %s", err.c_str());
    x -> tweetdata -> clear();
    return realsize;
  }

  outlet_anything((x -> out), gensym( json.serialize().c_str() ), 0, (t_atom *)NIL);
  x -> tweetdata -> clear();

  return realsize;


}

int    progressCallback( void       *userdata
                       , curl_off_t  dltotal
                       , curl_off_t  dlnow
                       , curl_off_t  ultotal
                       , curl_off_t ulnow)
{

  rayTwitterOAuth *x = reinterpret_cast<rayTwitterOAuth *>(userdata);

  return 0;
}


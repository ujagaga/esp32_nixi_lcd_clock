#ifndef HTTP_UI_H
#define HTTP_UI_H

/*
 *  Static HTML/CSS/JS page templates served by http_server.cpp.
 *  Kept here to keep the request handlers readable. The device serves a
 *  single config page: pick a WiFi AP (scanned live) + password, save. The
 *  SSID input is filled in dynamically by http_server.cpp between
 *  CONFIG_FORM_HEAD and CONFIG_FORM_TAIL.
 */

#include <pgmspace.h>
#include "config.h"

static const char HTML_BEGIN[] PROGMEM = R"(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0">
    <title>Nixie Clock Setup</title>
    <style>
      body { background:#11151c; color:#e6e9ef; font-family: system-ui, Arial, Helvetica, sans-serif; margin:0; }
      a{ color:#ff7a5c; }
      .contain{ width:100%; }
      .center_div{ margin:0 auto; width:92%; max-width:800px; position:relative; padding:1.2rem 0 4.5rem; }
    </style>
  </head>
  <body>
)";

static const char HTML_END[] PROGMEM = "</body></html>";

static const char CONFIG_HTML_0[] PROGMEM = R"(
<style>
  .c{text-align: center;}
  input,select{width:95%;padding:5px;font-size:1em;text-align:center;}
  body{text-align: left;}
  button{display:block;margin:1rem auto 0;border:0;border-radius:0.55rem;color:#fff;line-height:1;font-size:1rem;padding:0.6rem 1.6rem;background-color:#ff5a3c;}
  button:hover{background-color:#ff6f55;}
  #vm{width:100%;height:40vh;overflow-y:auto;margin-bottom:1em;}
  #pbar{width:100%;height:10px;background:#2a3340;border-radius:5px;overflow:hidden;margin-bottom:1em;}
  #pfill{height:100%;width:0;background:#1fa3ec;transition:width 10s linear;}
</style>
  <div class="contain">
    <div class="center_div">
)";

static const char CONFIG_HTML_1[] PROGMEM = R"(
      <h1 id='ttl'>Scanning...</h1>
      <div id='pbar'><div id='pfill'></div></div>
      <div id='vm'>
)";

// Between these two, http_server.cpp inserts the SSID input (prefilled with
// the current value), the password input, and the location <select> options.
static const char CONFIG_FORM_HEAD[] PROGMEM = R"(
      </div>
      <form method='get' action='wifisave'>
)";

static const char CONFIG_FORM_TAIL[] PROGMEM = R"(
        <br><button type='submit'>Save</button>
      </form>
     </div>
  </div>
<script>
  // The device starts a WiFi scan when this page is served; the progress bar
  // approximates the ~10s scan, then results are fetched (no live connection).
  function c(l){
    document.getElementById('s').value=l.innerText||l.textContent;
    document.getElementById('p').focus();
  }
  function done(){ document.getElementById('pbar').style.display='none'; }
  function load(tries){
    fetch('/aplist').then(function(r){return r.text();}).then(function(t){
      var list=t.split('|').filter(function(s){return s.length>0;});
      var vm=document.getElementById('vm');
      if(!list.length){
        if(tries>0){ setTimeout(function(){ load(tries-1); }, 2000); return; }
        done(); document.getElementById('ttl').innerHTML='No networks found.'; vm.innerHTML='';
        return;
      }
      done(); document.getElementById('ttl').innerHTML='Networks found:';
      var html='';
      for(var i=0;i<list.length;i++){
        html+='<span>'+(i+1)+": </span><a href='#p' onclick='c(this)'>" + list[i] + '</a><br>';
      }
      vm.innerHTML=html;
    }).catch(function(){
      if(tries>0){ setTimeout(function(){ load(tries-1); }, 2000); } else { done(); }
    });
  }
  setTimeout(function(){ document.getElementById('pfill').style.width='100%'; }, 50); // animate bar
  setTimeout(function(){ load(3); }, 10000);   // fetch after the bar fills, then retry
</script>
)";

static const char REDIRECT_HTML[] PROGMEM = R"(
<p id="tmr"></p>
<script>
  var c=6;
  function count(){
    var tmr=document.getElementById('tmr');
    if(c>0){
      c--;
      tmr.innerHTML="You will be redirected to home page in "+c+" seconds.";
      setTimeout('count()',1000);
    }else{
      window.location.href="/";
    }
  }
  count();
</script>
)";

#endif

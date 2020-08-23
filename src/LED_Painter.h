#ifndef LED_PAINTER_H
#define LED_PAINTER_H

#include <memory>

const char PAINTER_HTTP_HEAD[] PROGMEM            = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{v}</title>";
const char PAINTER_HTTP_STYLE[] PROGMEM           = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input[type=text]{width:95%;} input[type=radio] {width=30%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;}</style>";
const char PAINTER_HTTP_HEAD_END[] PROGMEM        = "</head><body><div style='text-align:left;display:inline-block;min-width:260px;'>";
const char PAINTER_HTTP_END[] PROGMEM             = "</div></body></html>";
const char PAINTER_HTTP_JS_IMAGE[] PROGMEM        = "<script>function setImage(elem) {var img = document.getElementById(\"PrevImg\"); img.src = elem.value; }</script>";



#endif
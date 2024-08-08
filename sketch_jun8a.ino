#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>
#include "HX711.h" // HX711 라이브러리 추가

// BLE 서비스 및 특성 UUID 정의
BLEService myService("00001101-0000-1000-8000-00805F9B34FB"); 
BLEStringCharacteristic myCharacteristic("00001101-0000-1000-8000-00805F9B34FB", BLERead | BLEWrite, 20); // 최대 20바이트 문자열

#define PIN 4 // RGB LED 스트립이 연결된 아두이노의 핀 번호
#define NUMPIXELS 18 // LED 스트립에 있는 LED 개수
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// HX711 모듈 핀 정의
#define LOADCELL_DOUT_PIN 3
#define LOADCELL_SCK_PIN 2

HX711 scale;

// dtostrf 함수 직접 구현
char* dtostrf(double val, signed char width, unsigned char prec, char *sout) {
  sprintf(sout, "%*.*f", width, prec, val);
  return sout;
}

void setup() {
  Serial.begin(9600); // 시리얼 통신 시작
  pixels.begin(); // NeoPixel 시작
  if (!BLE.begin()) { // BLE 초기화
    Serial.println("BLE 시작 실패");
    while (1);
  }

  BLE.setLocalName("하이루"); // BLE 디바이스 이름 설정
  BLE.setAdvertisedService(myService); // 광고할 서비스 추가
  myService.addCharacteristic(myCharacteristic); // 서비스에 특성 추가
  BLE.addService(myService); // BLE 서비스 추가
  
  myCharacteristic.writeValue("물좀마셔조"); // 초기 특성 값 설정

  BLE.advertise(); // BLE 광고 시작

  Serial.println("BLE 디바이스가 준비되었습니다. 이제 연결할 수 있습니다.");
  
  // 로드셀 초기화
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.tare(); // 시작 시 영점 조절

  // 스케일 팩터 설정(로드셀)
  scale.set_scale(-400100.00); // 400100.00

  // 초기 상태 (연결 대기 중) 노란색으로 설정
  setAllPixelsColor(255, 255, 0); // 노란색
}

void loop() {
  // BLE 중앙 장치와의 연결 대기
  BLEDevice central = BLE.central();

  // 중앙 장치가 연결되면
  if (central) {
    Serial.print("연결됨: ");
    Serial.println(central.address());
    // 연결되었을 때 초록색으로 설정
    setAllPixelsColor(0, 255, 0); // 초록색

    // 중앙 장치와 연결된 동안 기본색으로 유지
    while (central.connected()) {
      setAllPixelsColor(255, 115, 10); // 기본색
      
      // 실시간 무게를 읽고 시리얼 모니터에 출력
      float weight = scale.get_units(10); // 로드셀로부터 무게를 읽어옴, 10번 측정 후 평균값 사용
      char weightStr[10];
      dtostrf(weight, 1, 2, weightStr); // 무게를 문자열로 변환
      Serial.print("실시간 무게: ");
      Serial.print(weight);
      Serial.println(" kg"); // 단위를 kg으로 표시

      // 새로운 값을 쓰기 전에 중앙 장치에서 명령을 처리
      if (myCharacteristic.written()) {
        String value = myCharacteristic.value();
        Serial.print("새로운 값: ");
        Serial.println(value);
        
        if (value == "A") {
          // 영점 조절
          scale.tare();
          Serial.println("영점 조절 완료");
          setAllPixelsColor(0, 255, 0); // 핑크색
          delay(5000); // 5초 대기
          setAllPixelsColor(255, 115, 10); // 다시 기본색으로 변경
        } else if (value == "C") {
          Serial.println("목표치 달성"); // 시리얼 모니터에 "목표치 달성" 출력
          setAllPixelsColor(0, 255, 0); // LED를 초록색으로 변경
          delay(5000); // 5초 대기
          setAllPixelsColor(255, 115, 10); // 다시 기본색으로 변경
        } else if (value == "D") {
          // "물 추가"
          Serial.println("물 추가");
          setAllPixelsColor(0, 255, 0); // 하늘색
          delay(5000); // 5초 대기
          setAllPixelsColor(255, 115, 10); // 다시 기본색으로 변경
        } else if (value == "E") {
          // "물 버림"
          Serial.println("물 버림");
          setAllPixelsColor(0, 255, 0); // 노란색
          delay(5000); // 5초 대기
          setAllPixelsColor(255, 115, 10); // 다시 기본색으로 변경
        } else if (value == "B") {
          // "무게전송"
          Serial.println("무게 전송");
          setAllPixelsColor(0, 255, 0); // 주황
          delay(5000); // 5초 대기
          setAllPixelsColor(255, 115, 10); // 다시 기본색으로 변경
        }
      }

      // BLE 특성에 무게 전송
      myCharacteristic.writeValue(weightStr);

      delay(1000); // 1초 대기 (실시간 전송 간격 설정)
    }

    // 중앙 장치와의 연결이 끊어지면 붉은색으로 설정
    Serial.print("연결 해제: ");
    Serial.println(central.address());
    setAllPixelsColor(255, 0, 0); // 붉은색
  } else {
    // 중앙 장치가 연결되지 않았을 때 붉은색으로 설정
    setAllPixelsColor(255, 0, 0); // 붉은색
  }
}

// 모든 LED에 동일한 색상을 설정하는 함수
void setAllPixelsColor(byte red, byte green, byte blue) {
  for(int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
  }
  pixels.show();
}

# loadcell_comm

`loadcell_comm`은 **RS-485 기반 LoadCell 장치와의 통신을 담당하는 C++ 라이브러리**입니다.  
본 라이브러리는 **타 프로젝트에서 링크하여 사용하는 것을 전제로** 설계되었으며,  
통신 설정과 빌드/설치 방법을 가장 중요한 정보로 제공합니다.

---

## 목차

1. 통신 설정 (SerialConfig)
2. 빌드 및 설치 방법
3. 프로젝트 구성 및 디렉터리 구조
4. LoadCell485 개요
5. LoadCell485 사용 방법
6. LoadCellStatus 구조
7. 오류 처리
8. FAQ / 트러블슈팅

---

## 1. 통신 설정 (SerialConfig)

LoadCell 장치는 **RS-485 시리얼 통신**을 사용합니다.
아래 설정 값은 **필수이며 변경되지 않는 전제**입니다.

### 1.1 시리얼 통신 기본 설정

| 항목              | 값                               |
| ---------------- | -------------------------------- |
| Interface        | RS-485                           |
| Device (Linux)   | `/dev/ttyUSB0`                   |
| Device (udev)    | `/dev/WEIGHT_SENSOR`             |
| Baudrate         | **9600**                         |
| Data Bits        | **8**                            |
| Parity           | **None (N)**                     |
| Stop Bits        | **1**                            |
| Flow Control     | **None**                         |
| Byte Order       | **Big-endian (High byte first)** |

※ 실제 장치 사양에 따라 다를 수 있으므로 장치 매뉴얼을 우선합니다.

---

### 1.2 디바이스 파일 및 권한

시리얼 디바이스는 권한 문제로 열리지 않는 경우가 많습니다.

```bash
sudo usermod -a -G dialout $USER
```

적용 후 재로그인이 필요합니다.

---

## 2. 빌드 및 설치 방법

### 2.1 제공 빌드 스크립트

본 프로젝트는 **스크립트 기반 빌드/설치**를 제공합니다.

```bash
scripts/builder.sh
```

스크립트 특징:

* 기존 build 디렉터리를 항상 제거 후 재생성
* 빌드 성공 후에만 install 디렉터리를 정리하고 재설치
* 공유 라이브러리(.so) 기본 생성

---

### 2.2 기본 빌드 및 설치 절차

프로젝트 루트에서 다음 명령을 실행합니다.

```bash
./scripts/builder.sh
```

---

### 2.3 빌드 결과물

빌드 및 설치 후 생성되는 주요 결과물은 다음과 같습니다.

```
install/
├── include/loadcell_comm
│   ├── loadcell_485.h
│   ├── loadcell_status.h
│   └── SerialConfig.h
└── lib
    ├── libloadcell_comm.so
    ├── libloadcell_comm.so.1
    └── libloadcell_comm.so.1.0.0
```

---

### 2.4 타 프로젝트에서의 사용

* Include 경로: `install/include`  
* Link 라이브러리: `loadcell_comm`  

---

## 3. 프로젝트 구성 및 디렉터리 구조

```
.
├── scripts
│   └── builder.sh
├── source
│   ├── loadcell_comm
│   │   ├── loadcell_485.cpp
│   │   ├── loadcell_485.h
│   │   ├── loadcell_exception.cpp
│   │   ├── loadcell_exception.h
│   │   └── loadcell_status.h
│   ├── ring_buffer
│   └── serial_comm
│   │   └── SerialConfig.h
└── install
```

---

## 4. LoadCell485 개요

`LoadCell485`는 LoadCell 장치와의 **시리얼 수신 통신 및 프레임 파싱**을 담당하는 클래스입니다.

주요 책임:

* 시리얼 포트 열기/닫기
* 수신 데이터 누적
* 프레임 검출 및 파싱
* 파싱 결과를 `LoadCellStatus`로 제공

---

## 5. LoadCell485 사용 방법

### 5.1 기본 사용 흐름

1. `SerialConfig` 설정
2. `LoadCell485` 생성
3. `Open()`
4. 주기적으로 `RecvOnce()`
5. 종료 시 `Close()`

---

### 5.2 데이터 수신

```cpp
LoadCellStatus status;
ResultCode rc = loadcell.RecvOnce(status);
```

* 수신된 프레임이 없을 경우 `kNoFrame` 반환
* 프레임 파싱 성공 시 `kOk` 반환

---

## 6. LoadCellStatus 구조

`LoadCellStatus`는 LoadCell 장치로부터 수신한 **무게 값과 상태 정보**를 담는 구조체입니다.
`LoadCell485::RecvOnce()` 호출 시 파싱 결과로 채워집니다.

### 6.1 구조체 정의

```cpp
struct LoadCellStatus {
  double gross_weight;
  double right_weight;
  double left_weight;

  uint8_t right_battery_percent;
  uint8_t right_charge_status;
  uint8_t right_online_status;

  uint8_t left_battery_percent;
  uint8_t left_charge_status;
  uint8_t left_online_status;

  uint8_t gross_net_mark;
  uint8_t overload_mark;
  uint8_t out_of_tolerance_mark;
};
```

| 필드             | 설명             |
| -------------- | -------------- |
| `gross_weight` | 전체(Gross) 무게   |
| `right_weight` | 우측 LoadCell 무게 |
| `left_weight`  | 좌측 LoadCell 무게 |
| `right_battery_percent` | 우측 배터리 잔량 (0~100)          |
| `left_battery_percent`  | 좌측 배터리 잔량 (0~100)          |
| `right_charge_status`   | 우측 충전 상태 (0: 미충전, 1: 충전 중) |
| `left_charge_status`    | 좌측 충전 상태 (0: 미충전, 1: 충전 중) |
| `right_online_status` | 우측 센서 상태 (0: Online, 1: Offline, 2: Hardware failure) |
| `left_online_status`  | 좌측 센서 상태 (0: Online, 1: Offline, 2: Hardware failure) |
| `gross_net_mark`        | 무게 기준 (0: Gross, 1: Net)               |
| `overload_mark`         | 과부하 여부 (0: 정상, 1: Overload)            |
| `out_of_tolerance_mark` | 허용 오차 초과 여부 (0: 정상, 1: Left, 2: Right) |

---

## 7. 오류 처리

`LoadCell485::RecvOnce()`는 처리 결과를 `ResultCode`로 반환합니다.  
오류 발생 시, 추가적인 상세 원인은 `GetLastError()`를 통해 확인할 수 있습니다.  

---

### 7.1 ResultCode 목록

```cpp
enum class ResultCode {
  kOk = 0,
  kFrameTooShort = 1,
  kNoFrame = 2,
  kIoReadFail = 3
};
```

| ResultCode        | 설명                                                       |
| ----------------- | --------------------------------------------------------- |
| `kOk`             | 정상적으로 프레임을 수신하고 파싱함                             |
| `kFrameTooShort`  | 수신 버퍼에 프레임 길이(25 bytes)를 충족하는 데이터가 아직 없음   |
| `kNoFrame`        | 수신 버퍼에 완전한 프레임이 없음  |
| `kIoReadFail`     | 시리얼 포트 read 중 오류 발생 |

---

### 7.2 ResultCode 상세 설명

#### `kOk`

* 25바이트 고정 프레임의 헤더가 일치함
* 프레임 파싱이 완료되어 `LoadCellStatus`가 갱신됨

### `kFrameTooShort`

* 수신 버퍼에 누적된 데이터가 프레임 최소 길이(25 bytes)에 도달하지 않음
* 통신 오류가 아님
* 스트리밍/폴링 기반 수신 구조에서 정상적으로 발생 가능한 상태
👉 호출자는 다음 수신을 대기하면 됨

#### `kNoFrame`

* 다음 중 하나의 상황에 해당함:
  * 수신 버퍼 크기는 충분하나, 수신 버퍼 내에서 유효한 프레임 헤더(`0x55 0xAB 0x01`)를 찾지 못함
  * 잘못된 데이터, 동기 깨짐, 노이즈 등의 가능성
  * 일반적으로 주기적 polling 환경에서 정상적으로 발생 가능
  * 호출자는 이 값을 무시하고 다음 수신을 대기하면 됨

---

#### `kIoReadFail`

* 시리얼 포트에서 데이터를 읽는 과정에서 오류 발생
* 포트가 닫혔거나, 장치가 분리되었거나, OS 레벨 오류가 발생한 경우

---

### 7.3 상세 오류 메시지

오류 발생 시, 내부적으로 기록된 문자열 오류 메시지를 확인할 수 있습니다.

```cpp
const std::string& error = loadcell.GetLastError();
```

| 상황     | 예시 메시지                                 |
| ------ | -------------------------------------- |
| 데이터 부족 | `Not enough data in buffer (xx bytes)` |
| 헤더 미검출 | `No valid header found in buffer`      |
| IO 오류  | 시리얼 포트 드라이버 반환 오류                      |

이 메시지는 **디버깅 및 로그 기록 용도**로 제공되며,
오류 처리 정책(재시도, 재연결 등)은 상위 애플리케이션에서 결정하는 것을 전제로 합니다.

---

### 7.4 권장 처리 패턴

```cpp
LoadCellStatus status;
ResultCode rc = loadcell.RecvOnce(status);

switch (rc) {
  case ResultCode::kOk:
    // 정상 처리
    break;

  case ResultCode::kFrameTooShort:
  case ResultCode::kNoFrame:
    // 정상적인 대기 상태, 무시 가능
    break;

  case ResultCode::kIoReadFail:
    // 포트 상태 점검 또는 재연결 처리
    break;
}
```

---

## 8. FAQ / 트러블슈팅

### 8.1 통신이 되지 않는 경우

* 디바이스 파일 확인
* 권한(dialout 그룹) 확인
* 통신 설정 값 확인

### 8.2 프레임이 수신되지 않는 경우

* 헤더 패턴 확인
* 장치 송신 주기 확인

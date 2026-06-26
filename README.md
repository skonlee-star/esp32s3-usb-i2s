# ESP32-S3 Zero → USB Audio (UAC) → I2S → PCM5102A

PC에 USB로 연결하면 **드라이버 없이 USB 사운드카드**로 인식되고,  
I2S로 PCM5102A DAC에 오디오를 출력하는 펌웨어입니다.

---

## 핀 연결

| ESP32-S3 Zero | PCM5102A 모듈 | 설명 |
|---|---|---|
| **GPIO 1** | BCK | Bit Clock |
| **GPIO 2** | LCK | Word Select (LRCK) |
| **GPIO 3** | DIN | Data Input |
| GPIO 4 | SCK | MCLK (PCM5102 내부 PLL 사용 시 미연결 가능) |
| 3.3V | VCC, FLT, DEMP, XSMT | 전원 및 설정 핀 High |
| GND | GND, FMT | GND (FMT=GND → I2S 포맷) |

> PCM5102A 모듈을 사용할 경우 FLT/DEMP/XSMT/FMT는 보통 모듈에서  
> 이미 처리되어 있으므로 VCC·GND·BCK·LCK·DIN만 연결하면 됩니다.

---

## 펌웨어 다운로드 (자동 빌드)

이 저장소를 GitHub에 올리면 Actions가 자동으로 빌드합니다.

1. **Actions 탭** → 가장 최근 워크플로우 클릭
2. **Artifacts** 섹션에서 `esp32s3-uac-pcm5102-firmware` 다운로드
3. ZIP 압축 해제 → `esp32s3_uac_pcm5102_MERGED.bin` 사용

---

## 플래시 방법 (브라우저, 설치 불필요)

### 방법 A — Espressif 공식 웹 플래셔 (권장)

1. Chrome 또는 Edge 열기
2. `https://espressif.github.io/esptool-js` 접속
3. **ESP32-S3 Zero 보드를 다운로드 모드로 진입:**
   - BOOT 버튼을 누른 채 USB 케이블 연결 → BOOT 버튼 놓기
4. **Connect** 클릭 → COM 포트 선택
5. 파일 항목 설정:

   | 파일 | 주소 |
   |---|---|
   | `esp32s3_uac_pcm5102_MERGED.bin` | `0x0` |

6. **Program** 클릭 → 완료 후 보드 리셋

### 방법 B — ESPWebTool (더 심플한 UI)

1. `https://esptool.spacehuhn.com` 접속
2. 위와 동일하게 연결 후 merged bin 파일 선택, 주소 `0x0`

---

## 동작 확인

플래시 완료 후 USB를 다시 연결하면:

- Windows: 장치 관리자에 **"ESP32-S3 UAC Speaker"** 표시
- macOS: 시스템 환경설정 → 사운드에서 출력 장치 선택 가능
- Linux: `aplay -l` 에서 확인

---

## 오디오 사양

| 항목 | 값 |
|---|---|
| 샘플링 레이트 | 48,000 Hz |
| 비트 깊이 | 16-bit |
| 채널 | Stereo (2ch) |
| 인터페이스 | USB Audio Class (UAC) 1.0 |

---

## 직접 빌드

```bash
# ESP-IDF v5.3 환경 필요
git clone <이 저장소>
cd esp32s3-usb-i2s
idf.py set-target esp32s3
idf.py build

# Merged binary 생성
python3 $IDF_PATH/components/esptool_py/esptool/esptool.py \
  --chip esp32s3 merge_bin \
  -o build/merged.bin \
  --flash_mode qio --flash_size 4MB \
  0x0     build/bootloader/bootloader.bin \
  0x8000  build/partition_table/partition-table.bin \
  0x10000 build/esp32s3_usb_i2s.bin
```

# File Type Detector (File Signature-Based)

이 프로젝트는 리눅스 및 macOS 환경에서 C언어의 **이진 파일 입출력(Binary File I/O)** 을 활용하여 파일의 타입을 판별하는 보안 도구입니다.
단순 확장자 검사가 아닌, 파일 내부의 **Header(Magic Number)** 와 **Footer(Trailer)** 를 직접 검사하여 파일의 무결성과 실제 타입을 식별합니다.

특히 ZIP 기반의 포맷(ODT, DOCX, APK 등)처럼 매직 넘버가 동일한 경우, 가능한 모든 타입을 탐지하여 출력하도록 설계되었습니다.

## 파일 구성

| 파일명                | 설명                                                               |
| --------------------- | ------------------------------------------------------------------ |
| `signature.h`         | 공통 구조체(`SignatureRecord`) 및 상수 정의 Header 파일            |
| `file_sig_manager.c`  | 시그니처 데이터베이스(`sigdata.dat`)를 관리(CRUD)하는 도구         |
| `file_sig_detector.c` | DB를 기반으로 대상 파일을 분석하고 판별하는 도구                   |
| `sigdata.dat`         | 시그니처 정보가 저장되는 이진 DB 파일 (프로그램 실행 시 자동 생성) |

## 컴파일 방법 (Build)

터미널에서 `gcc`를 사용하여 두 도구를 컴파일합니다.

```bash
gcc -o file_sig_manager file_sig_manager.c
gcc -o file_sig_detectoror file_sig_detectoror.c
```

---

## 사용법 1: 시그니처 데이터 관리 (file_sig_manager)

`file_sig_manager`는 파일에서 직접 시그니처를 추출하여 DB에 등록하거나, 관리하는 역할을 합니다.

### 1\. 시그니처 추가 (Add)

대상 파일의 앞부분(Header)과 뒷부분(Footer)을 읽어 DB에 저장합니다.

```bash
# 사용법: ./file_sig_manager add <샘플파일> <시그니처명> [Footer길이]
```

- **예시 1 (Header만 등록):** 리눅스 실행 파일(ELF) 등 Footer가 가변적인 경우
  ```bash
  ./file_sig_manager add /bin/ls "Linux Executable" 0
  ```
- **예시 2 (Header+Footer 등록):** JPEG, PNG 등 끝부분이 명확한 경우
  ```bash
  # JPEG는 끝부분 2바이트(FF D9)가 Footer임
  ./file_sig_manager add sample.jpg "JPEG Image" 2
  ```

### 2\. 목록 조회 (List)

현재 등록된 시그니처 목록을 확인합니다.

```bash
./file_sig_manager list
```

### 3\. 정보 수정 (Update)

등록된 시그니처의 이름을 변경합니다.

```bash
# 사용법: ./file_sig_manager update <ID> <새이름>
./file_sig_manager update 1 "ELF Binary 64-bit"
```

### 4\. 삭제 (Delete)

특정 시그니처를 DB에서 비활성화(논리적 삭제) 합니다.

```bash
# 사용법: ./file_sig_manager delete <ID>
./file_sig_manager delete 2
```

---

## 사용법 2: 파일 타입 판별 (file_sig_detector)

`file_sig_detector`는 DB에 저장된 시그니처들과 대상 파일을 비교합니다. 하나의 파일이 여러 시그니처와 일치할 경우(예: ODT, ZIP, JAR) 모든 결과를 출력합니다.

```bash
# 사용법: ./file_sig_detector <검사할파일>
./file_sig_detector document.odt
```

### 판별 결과 해석

출력 결과는 다음 세 가지 상태로 나뉩니다.

1.  **[MATCH]**

    - Header와 Footer(정의된 경우)가 모두 정확히 일치합니다.
    - 파일이 정상적이며 해당 타입일 가능성이 매우 높습니다.

2.  **[PARTIAL MATCH] (Header OK, Footer Mismatch)**

    - Header(Magic Number)는 일치하지만, 파일 끝부분(Footer)이 다릅니다.
    - **보안 경고:** 파일이 전송 중 잘렸거나(Truncated), 파일 뒤에 악성 데이터가 삽입되었을(Steganography 등) 가능성이 있습니다.

3.  **[UNKNOWN]**

    - DB에 일치하는 시그니처가 없습니다.

---

## 활용 예시 (다중 탐지)

ODT, DOCX, APK 등은 모두 ZIP 포맷(`50 4B 03 04`)을 기반으로 합니다. 이들을 모두 등록해두면 다음과 같이 탐지됩니다.

```text
Analyzing: report.odt (2800 bytes)
--------------------------------------
[MATCH #1] ID: 5 | Name: ZIP Archive
[MATCH #2] ID: 6 | Name: OpenDocument Text
```

## 주의사항

- `sigdata.dat` 파일은 이진 파일이므로 텍스트 에디터로 직접 수정하지 마시고 반드시 `file_sig_manager`를 사용하세요.
- 이 도구는 파일의 \*\*처음(Header)\*\*과 \*\*끝(Footer)\*\*만 검사합니다. 파일 중간의 데이터 변조까지는 탐지하지 않습니다.

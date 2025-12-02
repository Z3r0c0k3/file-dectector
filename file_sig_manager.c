/*
 * 파일명: sig_maker.c
 * 기 능: 파일 시그니처 데이터베이스 관리 도구
 * 요구사항: 구조체, 포인터, 사용자 정의 함수, 상세 주석 포함
 */

#include "signature.h"

/*
 * 함수명: print_usage
 * 기 능: 프로그램 사용법을 출력
 * 인 자:
 * - prog_name: 실행 파일 이름 문자열 포인터
 */
void print_usage(const char *prog_name)
{
    printf("사용법:\n");
    printf("  %s add <대상파일> <시그니처명> [Footer길이]\n", prog_name);
    printf("  %s list\n", prog_name);
    printf("  %s update <ID> <새이름>\n", prog_name);
    printf("  %s delete <ID>\n", prog_name);
}

/*
 * 함수명: add_signature
 * 기 능: 대상 파일에서 시그니처를 추출하여 DB에 추가
 * 인 자:
 * - filepath: 대상 파일 경로
 * - sig_name: 저장할 시그니처 이름
 * - footer_bytes_to_read: 읽어올 Footer의 길이
 */
void add_signature(const char *filepath, const char *sig_name, int footer_bytes_to_read)
{
    // 1. 대상 파일 열기 (읽기 전용 이진 모드)
    FILE *target_fp = fopen(filepath, "rb");
    if (!target_fp)
    {
        perror("대상 파일 열기 실패"); // 에러 메시지 출력
        return;
    }

    // 파일 크기 계산
    fseek(target_fp, 0, SEEK_END);    // 포인터를 파일 끝으로 이동
    long filesize = ftell(target_fp); // 파일 크기 반환
    fseek(target_fp, 0, SEEK_SET);    // 포인터를 다시 파일 처음으로 원복

    // 구조체 변수 선언 및 초기화
    SignatureRecord record;
    memset(&record, 0, sizeof(SignatureRecord)); // 메모리를 0으로 초기화

    // 2. Header 읽기
    // 파일의 처음부터 최대 MAX_SIG_LEN 만큼 읽어서 구조체에 저장
    size_t h_read = fread(record.header, 1, MAX_SIG_LEN, target_fp);
    record.header_len = (int)h_read;

    // 3. Footer 읽기 (사용자가 요청 시)
    if (footer_bytes_to_read > 0)
    {
        // 최대 길이 제한
        if (footer_bytes_to_read > MAX_SIG_LEN)
            footer_bytes_to_read = MAX_SIG_LEN;

        // 파일 끝에서 footer_bytes_to_read 만큼 앞으로 이동
        fseek(target_fp, -footer_bytes_to_read, SEEK_END);

        size_t f_read = fread(record.footer, 1, footer_bytes_to_read, target_fp);
        record.footer_len = (int)f_read;
    }
    else
    {
        record.footer_len = 0; // Footer 없음
    }

    fclose(target_fp); // 대상 파일 닫기

    // 4. 구조체 데이터 채우기
    strncpy(record.name, sig_name, MAX_NAME_LEN - 1); // 이름 복사
    record.is_deleted = 0;                            // 삭제되지 않음으로 표시

    // 5. DB 파일에 저장
    // 'rb+' 모드로 열기 시도 (존재 시 수정/추가), 실패 시 'wb+'(생성)
    FILE *db_fp = fopen(DB_FILE, "rb+");
    if (!db_fp)
        db_fp = fopen(DB_FILE, "wb+");

    // DB 파일 끝으로 이동하여 ID 부여 및 데이터 쓰기
    fseek(db_fp, 0, SEEK_END);
    long fsize = ftell(db_fp);
    record.id = (fsize / sizeof(SignatureRecord)) + 1; // ID 자동 증가 로직

    // 구조체 전체를 이진 데이터로 파일에 쓰기
    fwrite(&record, sizeof(SignatureRecord), 1, db_fp);
    fclose(db_fp);

    printf("[추가됨] ID: %d, 이름: '%s'\n", record.id, record.name);
}

/*
 * 함수명: list_signatures
 * 기 능: 등록된 모든 시그니처 목록 조회
 */
void list_signatures()
{
    FILE *fp = fopen(DB_FILE, "rb");
    if (!fp)
    {
        printf("데이터베이스 파일이 없습니다.\n");
        return;
    }

    SignatureRecord record;
    printf("\nID\t이름\t\t\tHeader(Hex)\n");
    printf("----------------------------------------------------\n");

    // 파일 끝까지 구조체 단위로 반복해서 읽음
    while (fread(&record, sizeof(SignatureRecord), 1, fp))
    {
        if (record.is_deleted)
            continue; // 삭제된 항목은 건너뜀

        printf("%d\t%-20s\t", record.id, record.name);

        // Header 내용 16진수로 출력 (최대 8바이트까지만 표시)
        for (int i = 0; i < 8 && i < record.header_len; i++)
        {
            printf("%02X ", record.header[i]);
        }
        if (record.header_len > 8)
            printf("...");
        printf("\n");
    }
    fclose(fp);
    printf("\n");
}

/*
 * 함수명: update_signature
 * 기 능: 특정 ID의 시그니처 이름 수정
 * 인 자:
 * - id: 수정할 ID
 * - new_name: 새로운 이름 문자열
 */
void update_signature(int id, const char *new_name)
{
    FILE *fp = fopen(DB_FILE, "rb+"); // 수정 모드
    if (!fp)
        return;

    SignatureRecord record;
    int found = 0;

    while (fread(&record, sizeof(SignatureRecord), 1, fp))
    {
        // ID가 일치하고 삭제되지 않은 레코드 검색
        if (record.id == id && !record.is_deleted)
        {
            // 현재 파일 포인터는 레코드를 읽은 직후이므로,
            // 수정을 위해 포인터를 레코드 크기만큼 뒤로 이동시킴
            fseek(fp, -((long)sizeof(SignatureRecord)), SEEK_CUR);

            // 데이터 수정
            strncpy(record.name, new_name, MAX_NAME_LEN - 1);

            // 덮어쓰기
            fwrite(&record, sizeof(SignatureRecord), 1, fp);
            found = 1;
            printf("[수정됨] ID %d -> '%s'\n", id, new_name);
            break;
        }
    }
    if (!found)
        printf("ID %d번을 찾을 수 없습니다.\n", id);
    fclose(fp);
}

/*
 * 함수명: delete_signature
 * 기 능: 특정 ID의 시그니처 삭제
 * 인 자: id
 */
void delete_signature(int id)
{
    FILE *fp = fopen(DB_FILE, "rb+");
    if (!fp)
        return;

    SignatureRecord record;
    int found = 0;

    while (fread(&record, sizeof(SignatureRecord), 1, fp))
    {
        if (record.id == id && !record.is_deleted)
        {
            // 위치 되감기
            fseek(fp, -((long)sizeof(SignatureRecord)), SEEK_CUR);

            record.is_deleted = 1; // 논리적 삭제 플래그 설정

            // 변경사항 저장
            fwrite(&record, sizeof(SignatureRecord), 1, fp);
            found = 1;
            printf("[삭제됨] ID %d번.\n", id);
            break;
        }
    }
    if (!found)
        printf("ID %d번을 찾을 수 없습니다.\n", id);
    fclose(fp);
}

// 메인 함수: 인자 파싱 및 기능 분기
int main(int argc, char *argv[])
{
    // 인자 개수 부족 시 사용법 출력
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    // 문자열 비교를 통해 기능 선택
    if (strcmp(argv[1], "add") == 0)
    {
        if (argc < 4)
        {
            print_usage(argv[0]);
            return 1;
        }
        int f_len = 0;
        // 5번째 인자가 있으면 정수로 변환하여 Footer 길이로 사용
        if (argc >= 5)
            f_len = atoi(argv[4]);
        add_signature(argv[2], argv[3], f_len);
    }
    else if (strcmp(argv[1], "list") == 0)
    {
        list_signatures();
    }
    else if (strcmp(argv[1], "update") == 0)
    {
        if (argc != 4)
        {
            print_usage(argv[0]);
            return 1;
        }
        update_signature(atoi(argv[2]), argv[3]);
    }
    else if (strcmp(argv[1], "delete") == 0)
    {
        if (argc != 3)
        {
            print_usage(argv[0]);
            return 1;
        }
        delete_signature(atoi(argv[2]));
    }
    else
    {
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
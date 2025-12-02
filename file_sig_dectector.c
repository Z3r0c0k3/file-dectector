/*
 * 파일명: sig_detect.c
 * 기 능: 시그니처 DB를 기반으로 파일 타입을 판별 (Header/Footer 검사)
 * 요구사항: 구조체, 포인터, 다중 매칭 지원, 상세 주석
 */

#include "signature.h"

int main(int argc, char *argv[])
{
    // 실행 인자 확인
    if (argc < 2)
    {
        printf("사용법: %s <검사할 파일경로>\n", argv[0]);
        return 1;
    }

    // 1. 검사 대상 파일 열기
    FILE *target_fp = fopen(argv[1], "rb");
    if (!target_fp)
    {
        perror("대상 파일 열기 에러");
        return 1;
    }

    // 파일 크기 측정 (Footer 위치 계산 및 파일 유효성 검증용)
    fseek(target_fp, 0, SEEK_END);
    long file_size = ftell(target_fp);
    rewind(target_fp); // 파일 포인터를 다시 처음(0)으로 초기화

    // 대상 파일 Header 읽기 (DB의 최대 시그니처 길이만큼 미리 읽음)
    unsigned char file_header[MAX_SIG_LEN];
    fread(file_header, 1, MAX_SIG_LEN, target_fp);

    // 2. DB 파일 열기
    FILE *db_fp = fopen(DB_FILE, "rb");
    if (!db_fp)
    {
        printf("데이터베이스 파일(%s)이 없습니다.\n", DB_FILE);
        fclose(target_fp);
        return 1;
    }

    SignatureRecord record;
    int detected = 0;    // 탐지 여부 플래그
    int match_count = 0; // 매칭된 횟수 카운터

    printf("분석 중: %s (크기: %ld bytes)\n", argv[1], file_size);
    printf("--------------------------------------\n");

    // DB를 순차적으로 읽으며 대상 파일과 비교
    while (fread(&record, sizeof(SignatureRecord), 1, db_fp))
    {
        if (record.is_deleted)
            continue; // 삭제된 항목 패스

        // 비교할 길이 결정 (저장된 Header 길이와 8바이트 중 작은 값)
        // 보통 매직넘버는 4~8바이트가 핵심이므로 효율성을 위함
        int compare_len = (record.header_len > 8) ? 8 : record.header_len;

        // [1차 검사] 메모리 비교 (Header 확인)
        if (memcmp(file_header, record.header, compare_len) == 0)
        {

            int footer_match = 1; // Footer 일치 여부 확인용 변수 (기본값: 1)

            // [2차 검사] Footer가 정의된 시그니처인 경우
            if (record.footer_len > 0)
            {
                // 파일 크기가 (Header+Footer)보다 작으면 물리적으로 불가능하므로 불일치 처리
                if (file_size < (record.header_len + record.footer_len))
                {
                    footer_match = 0;
                }
                else
                {
                    unsigned char file_footer[MAX_SIG_LEN];

                    // 파일 포인터를 뒤에서부터 footer_len 만큼 앞으로 이동
                    fseek(target_fp, -record.footer_len, SEEK_END);
                    fread(file_footer, 1, record.footer_len, target_fp);

                    // Footer 메모리 비교
                    if (memcmp(file_footer, record.footer, record.footer_len) != 0)
                    {
                        footer_match = 0;
                    }
                }
            }

            // 결과 출력
            if (footer_match)
            {
                match_count++;
                printf("[매칭 #%d] ID: %d | 타입: %s\n", match_count, record.id, record.name);
                detected = 1;
            }
            else
            {
                // Header는 맞지만 Footer가 틀린 경우 -> 손상 가능성 알림
                printf("[부분 일치] ID: %d | 타입: %s (Header 일치, Footer 불일치)\n", record.id, record.name);
            }
        }
    }

    if (!detected)
        printf("[알 수 없음] 일치하는 시그니처가 없습니다.\n");

    // 리소스 해제
    fclose(db_fp);
    fclose(target_fp);
    return 0;
}
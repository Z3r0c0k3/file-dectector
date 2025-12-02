/*
 * 파일명: signature.h
 * 설 명: 프로그램 전반에서 사용하는 상수 및 구조체 정의 Header
 */
#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 데이터베이스 파일명 및 버퍼 크기 상수 정의
#define DB_FILE "sigdata.dat" // 시그니처 정보가 저장될 이진 파일명
#define MAX_SIG_LEN 16        // Header/Footer 시그니처의 최대 바이트 길이
#define MAX_NAME_LEN 64       // 시그니처 이름(파일 타입 명)의 최대 길이

/*
 * 구조체명: SignatureRecord
 * 설 명: 하나의 파일 형식에 대한 시그니처 정보를 담는 구조체
 */
typedef struct
{
    int id;                  // 고유 식별 번호 (1부터 시작)
    int is_deleted;          // 삭제 여부 플래그 (0: 정상, 1: 삭제됨)
    char name[MAX_NAME_LEN]; // 파일 형식 이름

    unsigned char header[MAX_SIG_LEN]; // 파일 Header 바이트 배열
    int header_len;                    // 실제 저장된 Header 길이

    unsigned char footer[MAX_SIG_LEN]; // 파일 Footer 바이트 배열
    int footer_len;                    // 실제 저장된 Footer 길이
} SignatureRecord;

#endif
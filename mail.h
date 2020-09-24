#pragma once

enum class mail_address
{
    A,
    B,
    C,
};

enum class mail_subject
{
    morning,
    evening,
    afternoon,
    night,
};

struct morning_greeting
{
    static constexpr mail_subject subject = mail_subject::morning;
    int m;
};
struct evening_greeting
{
    static constexpr mail_subject subject = mail_subject::evening;
    char message[16];
};
struct afternoon_greeting
{
    static constexpr mail_subject subject = mail_subject::afternoon;
    unsigned long param[4];
};
struct night_greeting
{
    static constexpr mail_subject subject = mail_subject::night;
    short option[8];
};

struct mail_header
{
    mail_address from;
    mail_address to;
    mail_subject subject;
};

union mail_body
{
    morning_greeting   morning;
    evening_greeting   evening;
    afternoon_greeting afternoon;
    night_greeting     night;
};

struct mail
{
    mail_header header;
    mail_body   body;
};

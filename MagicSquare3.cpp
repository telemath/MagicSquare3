// MagicSquare3.cpp : Search for a magic square of squares.
// The magic square is filled in with values A - I, which
// must all be perfect squares:
//
//  +---+---+---+
//  | A | B | C |
//  +---+---+---+
//  | D | E | F |
//  +---+---+---+
//  | G | H | I |
//  +---+---+---+
//
// This program relies on finding Magic Triples. A Magic Triple
// is three numbers, [l, e, s], such that l^2 + e^2 = 2 * s^2.
// This program selects one Magic Triple where [A = l^2, E = e^2, I = s^2]
// and another Magic Triple where [B = l^2, E = e^2, H = s^2].
// 
// The math behind "Magic Triple" approach is described at:
// https://www.solutionslookingforproblems.com/post/searching-for-a-3x3-magic-square-of-squares
// 
// A, B, E, H, and I are selected by selecting the Magic Triples.
// I refer to them in a few places below as Selected Numbers.
// 
// C, D, F, and G are calculated from A, B, and E, and must be perfect squares.
// I refer to them in a few places below as Calculated Numbers.


#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __BIGUNSIGNED_HPP__
#include "BigUnsigned.hpp"
#endif
#ifndef __BIGINTEGERUTILS_HPP__
#include "BigIntegerUtils.hpp"
#endif

// Enables checking F and D values for perfect squares and Magic Triple forms.
// Without this, the program will only check G and C values for perfect squares
// and Magic Triple forms. Runs faster without this defined.
#define __CHECK_FED__

// Reports G, C, F and D values that are perfect squares.
#define __REPORT_INDIVIDUAL_SQUARES__

const int MSG_LEN = 3000;
const int NUM_THREADS = 10;
const int NUM_STR_LEN = 25;
const int BIGINT_STR_LEN = 300;

// Counts to be reported.
unsigned long long g_g_squares = 0;
unsigned long long g_ge_magictriples = 0;
unsigned long long g_c_squares = 0;
unsigned long long g_ce_magictriples = 0;
unsigned long long g_gc_squares = 0;
unsigned long long g_gec_magictriples = 0;
#ifdef  __CHECK_FED__
unsigned long long g_f_squares = 0;
unsigned long long g_fe_magictriples = 0;
unsigned long long g_d_squares = 0;
unsigned long long g_de_magictriples = 0;
unsigned long long g_fd_squares = 0;
unsigned long long g_fed_magictriples = 0;
#endif
unsigned long long g_errors = 0;
unsigned long long g_batch_attempts = 0;
unsigned long long g_total_attempts = 0;
BigInteger min_e = 0;

// A square, mod 10, cannot end in a 2, 3, 7, or 8. A quick mod 10 check,
// will eliminate 40% of all random numbers as possible squares.
// Here, we use mod 44352, where only 2.597% of remainders can be squares.
static const int POSSIBLE_SQUARES_MODULUS = 44352;
static bool possible_squares [POSSIBLE_SQUARES_MODULUS];

// Get the integer root of BigInteger, or the closest number to the integer root.
// Tell whether the given root is the root of a perfect square.
static BigInteger IntegerRoot(BigInteger &n, bool *is_perfect_root);


/**********************************************************************
 **** Mutex functions                                              ****
 *********************************************************************/

 #ifdef __linux__ 
    #include <sys/time.h>
    #include <pthread.h>

    static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

    static void lock_mutex() {
        pthread_mutex_lock(&thread_mutex);
    }

    static void unlock_mutex() {
        pthread_mutex_unlock(&thread_mutex);
    }   

#elif _WIN32

    #include <windows.h>
    #include <process.h>

    CRITICAL_SECTION g_OutputLock;

    static void lock_mutex() {
        EnterCriticalSection(&g_OutputLock);
    }

    static void unlock_mutex() {
        LeaveCriticalSection(&g_OutputLock);
    }   

#else
    #error "Platform not supported"
#endif

 
 /**********************************************************************
 **** Output functions                                             ****
 *********************************************************************/


 // Output to stdout and to a text file.
static void LogMessage(const char* message, const char* filename)
{
    lock_mutex();

    fprintf(stdout, "%s\n", message);

    #ifdef __linux__
        FILE* fp = fopen(filename, "a");
    #elif _WIN32
        FILE* fp = NULL;
        fopen_s(&fp, filename, "a");
    #endif

    if (fp == 0) {
        unlock_mutex();
        return;
    }

    fprintf(fp, "%s\n", message);
    fclose(fp);
    fp = NULL;

    unlock_mutex();
}


// Format an unsigned long long with commas as thousands-separators.
static void FormatWithCommas(unsigned long long value, char* output, int output_len)
{   
    int cluster0 = value % 1000LL;
    int cluster1 = (value / 1000LL) % 1000LL;
    int cluster2 = (value / 1000000LL) % 1000LL;
    int cluster3 = (value / 1000000000LL) % 1000LL;
    int cluster4 = (value / 1000000000000LL) % 1000LL;
    if (cluster4 > 0) {
        snprintf(output, output_len,"%i,%03i,%03i,%03i,%03i", cluster4, cluster3, cluster2, cluster1, cluster0);
    }
    else if (cluster3 > 0) {
        snprintf(output, output_len, "%i,%03i,%03i,%03i", cluster3, cluster2, cluster1, cluster0);
    }
    else if (cluster2 > 0) {
        snprintf(output, output_len, "%i,%03i,%03i", cluster2, cluster1, cluster0);
    }
    else if (cluster1 > 0) {
        snprintf(output, output_len, "%i,%03i", cluster1, cluster0);
    }
    else {
        snprintf(output, output_len, "%i", cluster0);
    }
}


// Format a BigInteger with commas as thousands-separators.
static void FormatBigIntWithCommas(BigInteger &i, char * output, int output_len)
{
    std::string num_str = bigIntegerToString(i);
    int len = (int) num_str.length();
    int comma_count = (len - 1) / 3;
    int result_len = len + comma_count;
    
    if (result_len >= output_len) {
        // Output buffer too small, just copy what fits
        size_t copy_len = static_cast<size_t>(output_len) - 1;
        #ifdef __linux__
            strncpy(output, num_str.c_str(), copy_len);
        #elif _WIN32
            strncpy_s(output, copy_len, num_str.c_str(), copy_len);
        #endif
        output[output_len - 1] = '\0';
        return;
    }
    
    int src_pos = len - 1;
    int dst_pos = result_len;
    output[dst_pos] = '\0';
    --dst_pos;
    
    int digit_count = 0;
    while (src_pos >= 0) {
        if (digit_count == 3) {
            output[dst_pos] = ',';
            --dst_pos;
            digit_count = 0;
        }
        output[dst_pos] = num_str[src_pos];
        --dst_pos;
        --src_pos;
        ++digit_count;
    }
}


// Report if any of the Calculated Numbers are perfect squares or in Magic Triple form.
static void ReportResults(unsigned long long j_a, unsigned long long k_a,
                          unsigned long long j_b, unsigned long long k_b,
                          BigInteger &l_a, BigInteger &e_a, BigInteger &l_b, BigInteger &e_b,
                          BigInteger  &L, BigInteger &S, const char * l_name, const char * s_name,
                          bool l_square, bool s_square, bool le_magictriple, bool se_magictriple)
{
    char message[MSG_LEN * 2];

    #ifdef __REPORT_INDIVIDUAL_SQUARES__
        if (l_square) {
            char j_a_str[NUM_STR_LEN];
            char k_a_str[NUM_STR_LEN];
            char j_b_str[NUM_STR_LEN];
            char k_b_str[NUM_STR_LEN];
            char l_a_str[BIGINT_STR_LEN];
            char e_a_str[BIGINT_STR_LEN];
            char l_b_str[BIGINT_STR_LEN];
            char e_b_str[BIGINT_STR_LEN];
            char L_str[BIGINT_STR_LEN];
            bool is_perfect_root = false;
            FormatWithCommas(j_a, j_a_str, NUM_STR_LEN);
            FormatWithCommas(k_a, k_a_str, NUM_STR_LEN);
            FormatWithCommas(j_b, j_b_str, NUM_STR_LEN);
            FormatWithCommas(k_b, k_b_str, NUM_STR_LEN);
            FormatBigIntWithCommas(l_a, l_a_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(e_a, e_a_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(l_b, l_b_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(e_b, e_b_str, BIGINT_STR_LEN);
            BigInteger l_root = IntegerRoot(L, &is_perfect_root);
            FormatBigIntWithCommas(l_root, L_str, BIGINT_STR_LEN);

            snprintf(message, MSG_LEN * 2, "At [Seeds: j_a=%s k_a=%s j_b=%s k_b=%s], [Entries: l_a=%s e_a=%s l_b=%s e_b=%s], %s=%s^2.",
                    j_a_str, k_a_str, j_b_str, k_b_str, l_a_str, e_a_str, l_b_str, e_b_str, l_name, L_str);
            LogMessage(message, "Results.txt");
        }
        if (s_square) {
            char j_a_str[NUM_STR_LEN];
            char k_a_str[NUM_STR_LEN];
            char j_b_str[NUM_STR_LEN];
            char k_b_str[NUM_STR_LEN];
            char l_a_str[BIGINT_STR_LEN];
            char e_a_str[BIGINT_STR_LEN];
            char l_b_str[BIGINT_STR_LEN];
            char e_b_str[BIGINT_STR_LEN];
            char S_str[BIGINT_STR_LEN];
            bool is_perfect_root = false;
            FormatWithCommas(j_a, j_a_str, NUM_STR_LEN);
            FormatWithCommas(k_a, k_a_str, NUM_STR_LEN);
            FormatWithCommas(j_b, j_b_str, NUM_STR_LEN);
            FormatWithCommas(k_b, k_b_str, NUM_STR_LEN);
            FormatBigIntWithCommas(l_a, l_a_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(e_a, e_a_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(l_b, l_b_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(e_b, e_b_str, BIGINT_STR_LEN);
            BigInteger s_root = IntegerRoot(S, &is_perfect_root);
            FormatBigIntWithCommas(s_root, S_str, BIGINT_STR_LEN);

            snprintf(message, MSG_LEN * 2, "At [Seeds: j_a=%s k_a=%s j_b=%s k_b=%s], [Entries: l_a=%s e_a=%s l_b=%s e_b=%s], %s=%s^2.",
                    j_a_str, k_a_str, j_b_str, k_b_str, l_a_str, e_a_str, l_b_str, e_b_str, s_name, S_str);
            LogMessage(message, "Results.txt");
        }
    #endif

    // I think these conditions are impossible to meet, but have not proven it.
    // A proof would close the 3x3 Magic Square of Squares problem.
    //
    // This section has a lot of sanity checks in case I'm wrong about these statements:
    //     If l and s are squares, they must form a Magic Triple with e.
    //     If l and e are in magic triple form, S must also be part of the Magic Triple and therefore a square.
    //     If s and e are in magic triple form, L must also be part of the Magic Triple and therefore a square.
    if ((l_square && s_square) || le_magictriple || se_magictriple)
    {
        BigInteger E = e_a * e_a * e_b * e_b;
        char j_a_str[NUM_STR_LEN];
        char k_a_str[NUM_STR_LEN];
        char j_b_str[NUM_STR_LEN];
        char k_b_str[NUM_STR_LEN];
        char l_a_str[BIGINT_STR_LEN];
        char e_a_str[BIGINT_STR_LEN];
        char l_b_str[BIGINT_STR_LEN];
        char e_b_str[BIGINT_STR_LEN];
        char L_str[BIGINT_STR_LEN];
        char S_str[BIGINT_STR_LEN];
        char E_str[BIGINT_STR_LEN];
        char message_start[MSG_LEN];
        FormatWithCommas(j_a, j_a_str, NUM_STR_LEN);
        FormatWithCommas(k_a, k_a_str, NUM_STR_LEN);
        FormatWithCommas(j_b, j_b_str, NUM_STR_LEN);
        FormatWithCommas(k_b, k_b_str, NUM_STR_LEN);
        FormatBigIntWithCommas(l_a, l_a_str, BIGINT_STR_LEN);
        FormatBigIntWithCommas(e_a, e_a_str, BIGINT_STR_LEN);
        FormatBigIntWithCommas(l_b, l_b_str, BIGINT_STR_LEN);
        FormatBigIntWithCommas(e_b, e_b_str, BIGINT_STR_LEN);
        FormatBigIntWithCommas(L, L_str, BIGINT_STR_LEN);
        FormatBigIntWithCommas(S, S_str, BIGINT_STR_LEN);
        FormatBigIntWithCommas(E, E_str, BIGINT_STR_LEN);
        snprintf(message_start, MSG_LEN, "At [Seeds: j_a=%s k_a=%s j_b=%s k_b=%s], [Entries: l_a=%s e_a=%s l_b=%s e_b=%s]",
                j_a_str, k_a_str, j_b_str, k_b_str, l_a_str, e_a_str, l_b_str, e_b_str);

        if (l_square && s_square)
        {
            snprintf(message, MSG_LEN * 2, "%s, %s=%s and %s=%s are squares.",
                    message_start, l_name, L_str, s_name, S_str);
            LogMessage(message, "Results.txt");

            if (!le_magictriple)
            {
                snprintf(message, MSG_LEN * 2, "%s, %s=%s and %s=%s are squares, but %s and E=%s are not in Magic Triple form.",
                        message_start, l_name, L_str, s_name, S_str, l_name, E_str);
                LogMessage(message, "Results.txt");

                lock_mutex();
                ++g_errors;
                unlock_mutex();
            }
            if (!se_magictriple)
            {
                snprintf(message, MSG_LEN * 2, "%s, %s=%s and %s=%s are squares, but %s and E=%s are not in Magic Triple form.",
                        message_start, l_name, L_str, s_name, S_str, s_name, E_str);
                LogMessage(message, "Results.txt");
                lock_mutex();
                ++g_errors;
                unlock_mutex();
            }
        }
        if (le_magictriple) {
            snprintf(message, MSG_LEN * 2, "%s, %s=%s and E=%s are in Magic Triple form.",
                    message_start, l_name, L_str, E_str);
            LogMessage(message, "Results.txt");

            if (!(l_square && s_square))
            {
                snprintf(message, MSG_LEN * 2, "%s, %s=%s and E=%s are in Magic Triple form, but %s=%s and %s=%s are not both squares.",
                        message_start, l_name, L_str, E_str, l_name, L_str, s_name, S_str);
                LogMessage(message, "Results.txt");
                lock_mutex();
                ++g_errors;
                unlock_mutex();
            }
        }
        if (se_magictriple)
        {
            snprintf(message, MSG_LEN * 2, "%s, %s=%s and E=%s are in Magic Triple form.",
                    message_start, s_name, S_str, E_str);
            LogMessage(message, "Results.txt");

            if (!(l_square && s_square)) {
                snprintf(message, MSG_LEN * 2, "%s, %s=%s and E=%s are in Magic Triple form, but %s=%s and %s=%s are not both squares.",
                    message_start, s_name, S_str, E_str, l_name, L_str, s_name, S_str);
                LogMessage(message, "Results.txt");
                lock_mutex();
                ++g_errors;
                unlock_mutex();
            }
        }
    }
}


// Report the numbers collected:
//     Number of magic squares tried
//     Number of Calculated Numbers that are squares
//     Any Magic Triples resulting from Calculated Numbers (I don't expect to find any).
//     Errors (there'd better not be any - indicates a problem with my math or my code).
static void ReportProgress(unsigned long long j_a)
{
    char message[MSG_LEN];

    char j_a_str[NUM_STR_LEN];
    char batch_attempts_str[NUM_STR_LEN];
    char total_attempts_str[NUM_STR_LEN];
    char g_g_squares_str[NUM_STR_LEN];
    char g_ge_magictriples_str[NUM_STR_LEN];
    char g_c_squares_str[NUM_STR_LEN];
    char g_ce_magictriples_str[NUM_STR_LEN];
    char g_gec_magictriples_str[NUM_STR_LEN];
    char g_gc_squares_str[NUM_STR_LEN];
    char g_errors_str[NUM_STR_LEN];
    char date_and_time[100];

    FormatWithCommas(j_a, j_a_str, NUM_STR_LEN);
    FormatWithCommas(g_batch_attempts, batch_attempts_str, NUM_STR_LEN);
    FormatWithCommas(g_total_attempts, total_attempts_str, NUM_STR_LEN);
    FormatWithCommas(g_g_squares, g_g_squares_str, NUM_STR_LEN);
    FormatWithCommas(g_c_squares, g_c_squares_str, NUM_STR_LEN);
    FormatWithCommas(g_ge_magictriples, g_ge_magictriples_str, NUM_STR_LEN);
    FormatWithCommas(g_ce_magictriples, g_ce_magictriples_str, NUM_STR_LEN);
    FormatWithCommas(g_gec_magictriples, g_gec_magictriples_str, NUM_STR_LEN);
    FormatWithCommas(g_gc_squares, g_gc_squares_str, NUM_STR_LEN);
    FormatWithCommas(g_errors, g_errors_str, NUM_STR_LEN);

    // Format current date and time
    time_t now = time(nullptr);
    #ifdef __linux__
        struct tm* timeinfo = localtime(&now);
        strftime(date_and_time, sizeof(date_and_time), "%m-%d %H:%M", timeinfo);
    #elif _WIN32
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        strftime(date_and_time, sizeof(date_and_time), "%m-%d %H:%M", &timeinfo);
    #endif
    
    #ifdef __CHECK_FED__
        char g_f_squares_str[NUM_STR_LEN];
        char g_d_squares_str[NUM_STR_LEN];
        char g_fe_magictriples_str[NUM_STR_LEN];
        char g_de_magictriples_str[NUM_STR_LEN];
        char g_fed_magictriples_str[NUM_STR_LEN];
        char g_fd_squares_str[NUM_STR_LEN];
        FormatWithCommas(g_f_squares, g_f_squares_str, NUM_STR_LEN);
        FormatWithCommas(g_d_squares, g_d_squares_str, NUM_STR_LEN);
        FormatWithCommas(g_fe_magictriples, g_fe_magictriples_str, NUM_STR_LEN);
        FormatWithCommas(g_de_magictriples, g_de_magictriples_str, NUM_STR_LEN);
        FormatWithCommas(g_fed_magictriples, g_fed_magictriples_str, NUM_STR_LEN);
        FormatWithCommas(g_fd_squares, g_fd_squares_str, NUM_STR_LEN);

        snprintf(message, MSG_LEN,
                "%s: j_a=%s, %s tried, %s total, "
                "Squares: %s G, %s C, %s F, %s D, %s GC, %s FD, "
                "Triples: %s GE, %s CE, %s GEC, %s FE, %s DE, %s FED, "
                "%s errs.",
                date_and_time, j_a_str, batch_attempts_str, total_attempts_str,
                g_g_squares_str, g_c_squares_str, g_f_squares_str, g_d_squares_str, g_gc_squares_str, g_fd_squares_str,
                g_ge_magictriples_str, g_ce_magictriples_str, g_gec_magictriples_str, g_fe_magictriples_str, g_de_magictriples_str, g_fed_magictriples_str,
                g_errors_str);
    #else
        snprintf(message, MSG_LEN,
                "j_a=%s, %s attempts, %s total attempts, "
                "Squares: %s G, %s C, %s GC, "
                "Magic Triples: %s GE, %s CE, %s GEC, "
                "%s errors.",
                j_a_str, batch_attempts_str, total_attempts_str,
                g_g_squares_str, g_c_squares_str, g_gc_squares_str,
                g_ge_magictriples_str, g_ce_magictriples_str, g_gec_magictriples_str,
                g_errors_str);
    #endif

    LogMessage(message, "Progress.txt");
}


// Write a shell script or batch file to resume running if the program is halted.
static void WriteResumeScript(unsigned long long j_a)
{
    #ifdef __linux__
        const char * filename = "Resume.sh";
        const char * command = "./MagicSquare";
    #elif _WIN32
        const char * filename = "Resume.bat";
        const char * command = "MagicSquare";
    #endif

    char message[MSG_LEN];

    #ifdef __CHECK_FED__
        snprintf(message, MSG_LEN,
                "%s %llu %llu %llu %llu %llu %llu",
                command, j_a, g_total_attempts, g_g_squares, g_c_squares, g_f_squares, g_d_squares);
    #else
        snprintf(message, MSG_LEN,
                "%s %i %llu %llu %llu",
                command, j_a, g_total_attempts, g_g_squares, g_c_squares);
    #endif

    #ifdef __linux__
        FILE* fp = fopen(filename, "w");
    #elif _WIN32
        FILE* fp = NULL;
        fopen_s(&fp, filename, "w");
    #endif

    if (fp == 0) {
        return;
    }

    fprintf(fp, "%s\n", message);
    fclose(fp);
    fp = NULL;
}


/**********************************************************************
 **** Calculation functions                                        ****
 *********************************************************************/


// Create a BigInteger from an unsigned long long.
static BigInteger BigIntFromULL(unsigned long long x)
{
    BigInteger result = (uint32_t) (x >> 32);
    result *= 0x10000;
    result *= 0x10000;
    result += (uint32_t)(x & 0xFFFFFFFF);
    return result;
}


// Tell if two numbers are relatively prime.
static bool RelativelyPrime(unsigned long long a, unsigned long long b)
{
    if ((1 == a) || (1 == b))
    {
        return true;
    }
    if ((0 == a) || (0 == b))
    {
        return false;
    }

    while (true)
    {
        a %= b;
        if (1 == a) {
            return true;
        }
        if (0 == a) {
            return false;
        }
        b %= a;
        if (1 == b) {
            return true;
        }
        if (0 == b) {
            return false;
        }
    }
}

// Get the integer root of BigInteger, or the closest number to the integer root.
static BigInteger IntegerRoot(BigInteger& n, bool *is_perfect_root)
{
    if (NULL != is_perfect_root)
    {
        *is_perfect_root = false;
    }

    // First guess is half the length of n.
    int len = n.getLength();
    BigInteger root = 1;
    for (int i = 0; i < len; ++i)
    {
        root *= 0x10000;
    }

    // Newton's method for square root:
    BigInteger prev_root;
    BigInteger diff;
    BigInteger halfn = n / 2;

    do
    {
        prev_root = root;
        root = prev_root / 2 + halfn / prev_root;
        diff = root > prev_root ? root - prev_root : prev_root - root;
    } while ((diff != 0) && (diff != 1));

    // These checks suck so much, but I spent too much time flailing
    // about with this when I just wanted to see some results.

    // Try the final root.
    if (root * root == n) {
        if (NULL != is_perfect_root)
        {
            *is_perfect_root = true;
        }
        return root;
    }

    // Try the final root, plus 1.
    root += 1;
    if (root * root == n)
    {
	    if (NULL != is_perfect_root)
        {
            *is_perfect_root = true;
        }
        return root;
    }

    // Try the final root, minus 1.
    root -= 2;
    if (root * root == n)
    {
        if (NULL != is_perfect_root)
        {
            *is_perfect_root = true;
        }
        return root;
    }
    
    // Restore the final root.
    root += 1;
    return (prev_root == root) ? root : prev_root;
}


// Tell if a number can possibly be a perfect square.
static bool IsPossibleSquare(BigInteger N) 
{
    // See if the number, mod POSSIBLE_SQUARES_MODULUS, can be a square.
    int last_digits = (N % POSSIBLE_SQUARES_MODULUS).toInt();
    return possible_squares[last_digits];
}


// Tell if a BigInteger is a perfect square.
static bool IsPerfectSquare(BigInteger &n)
{
    // Negative numbers can not be perfect squares.
    if (n.getSign() == BigInteger::negative)
    {
        return false;
    }

    // Quick check to eliminate a lot of possibilities.
    if (!IsPossibleSquare(n))
    {
        return false;
    }

    // Do it the hard way.
    bool is_perfect_root = false;
    BigInteger root = IntegerRoot(n, &is_perfect_root);
    return is_perfect_root;
}


// Tell if two numbers are in Magic Triple form.
static bool IsMagicTripleForm(BigInteger &a, BigInteger &e)
{
    // See if a is of the form (j^2 + 2jk - k^2) and e is of the form (j^2 + k^2).
    bool is_perfect_root = false;
    BigInteger half_e = e / 2;
    BigInteger quarter_e = half_e / 2;
    BigInteger max_j = IntegerRoot(half_e, &is_perfect_root) + 1;
    for (BigInteger j = 1; j <= max_j; ++j)
    {
        BigInteger j_sq = j * j;
        BigInteger k_sq = e - j_sq;
        if (k_sq.getSign() != BigInteger::positive)
        {
            break;
        }
        bool is_perfect_root = false;
        BigInteger k = IntegerRoot(k_sq, &is_perfect_root);
        if (is_perfect_root) // If true, e = j^2 + k^2.
        {
            BigInteger jk2 = j * k * 2;
            BigInteger try_a = j_sq + jk2 - k_sq;
            if (a == try_a)
            {
                return true;
            }
            try_a = k_sq + jk2 - j_sq;
            if (a == try_a)
            {
                return true;
            }
        }
    }

    return false;
}


/**********************************************************************
 **** Trying Magic Square Values                                   ****
 *********************************************************************/


// Try Magic Triples selected for A-E-I and B-E-H. Report if any Calculated
// Numbers are squares or in Magic Triple form.
static void TryAandB(unsigned long long j_a, unsigned long long k_a,
                     unsigned long long j_b, unsigned long long k_b,
                     BigInteger &la, BigInteger &ea, BigInteger &lb, BigInteger &eb)
{
    // When testing G and C, A and B can be swapped and still yield the same results, so we only need to test once.
    BigInteger A = la * eb;
    A *= A;
    BigInteger B = lb * ea;
    B *= B;
    BigInteger e = ea * eb;
    BigInteger E = e * e;
    BigInteger E3 = E * 3;
    BigInteger G = A + B - E;
    BigInteger C = E3 - A - B;

    bool g_square = false;
    bool c_square = false;
    if ((G.getSign() == BigInteger::positive) && (C.getSign() == BigInteger::positive))
    {
        BigInteger g = 0;
        BigInteger c = 0;
        if (IsPossibleSquare(G)) {
            g = IntegerRoot(G, &g_square);
        }
        if (IsPossibleSquare(C)) {
            c = IntegerRoot(C, &c_square);
        }
       
        bool ge_magictriple = false;
        bool ce_magictriple = false;

        if (g_square)
        {
            ge_magictriple = IsMagicTripleForm(g, e);

            lock_mutex();
            ++g_g_squares;
            if (ge_magictriple)
            {
                ++g_ge_magictriples;
            }
            unlock_mutex();
        }

        if (c_square)
        {
            ce_magictriple = IsMagicTripleForm(c, e);

            lock_mutex();
            ++g_c_squares;
            if (ce_magictriple)
            {
                ++g_ce_magictriples;
            }
            unlock_mutex();
        }

        // These will likely never happen. I believe they're impossible, but not proven it.
        // A proof would close the Magic Square of Squares problem for 3x3 squares.
        if (g_square && c_square)
        {
            lock_mutex();
            ++g_gc_squares;
            unlock_mutex();
        }
        if (ge_magictriple && ce_magictriple)
        {
            lock_mutex();
            ++g_gec_magictriples;
            unlock_mutex();
        }

        ReportResults(j_a, k_a, j_b, k_b, la, ea, lb, eb, G, C, "G", "C", g_square, c_square, ge_magictriple, ce_magictriple);
    }


    #ifdef __CHECK_FED__

        // When Testing F and D, A and B we will get a different result if we swap A and B.
        // Try the original A nd B values here, then try swapping them below.
        BigInteger D = E3 - A - G;
        BigInteger F = E3 - D - E;

        if ((D.getSign() == BigInteger::positive) && (F.getSign() == BigInteger::positive))
        {
            bool f_square = false;
            bool d_square = false;
            BigInteger f = 0;
            BigInteger d = 0;
            if (IsPossibleSquare(F))
            {
                f = IntegerRoot(F, &f_square);
            }
            if (IsPossibleSquare(D))
            {
                d = IntegerRoot(D, &d_square);
            }

            bool fe_magictriple = false;
            bool de_magictriple = false;

            if (f_square)
            {
                fe_magictriple = IsMagicTripleForm(f, e);

                lock_mutex();
                ++g_f_squares;
                if (fe_magictriple)
                {
                    ++g_fe_magictriples;
                }
                unlock_mutex();
            }

            if (d_square)
            {
                de_magictriple = IsMagicTripleForm(d, e);

                lock_mutex();
                ++g_d_squares;
                if (de_magictriple)
                {
                    ++g_de_magictriples;
                }
                unlock_mutex();
            }

            // These will likely never happen. I believe they're impossible, but not proven it.
            // A proof would close the Magic Square of Squares problem for 3x3 squares.
            if (f_square && d_square)
            {
                lock_mutex();
                ++g_fd_squares;
                unlock_mutex();
            }
            if (fe_magictriple && de_magictriple)
            {
                lock_mutex();
                ++g_fed_magictriples;
                unlock_mutex();
            }

            ReportResults(j_a, k_a, j_b, k_b, la, ea, lb, eb, F, D, "F", "D", f_square, d_square, fe_magictriple, de_magictriple);
            
            if ((g_square || c_square) && (f_square || d_square))
            {
                LogMessage("    NOTE: Squares achieved along GEC and FED", "Results.txt");
            }
        }

        // For F and D, try swapping A and B.
        D = E3 - B - G;
        F = E3 - D - E;

        if ((D.getSign() == BigInteger::positive) && (F.getSign() == BigInteger::positive))
        {
            bool f_square = false;
            bool d_square = false;
            BigInteger f = 0;
            BigInteger d = 0;
            if (IsPossibleSquare(F))
            {
                f = IntegerRoot(F, &f_square);
            }
            if (IsPossibleSquare(D))
            {
                d = IntegerRoot(D, &d_square);
            }

            bool fe_magictriple = false;
            bool de_magictriple = false;

            if (f_square)
            {
                fe_magictriple = IsMagicTripleForm(f, e);

                lock_mutex();
                ++g_f_squares;
                if (fe_magictriple)
                {
                    ++g_fe_magictriples;
                }
                unlock_mutex();
            }

            if (d_square)
            {
                de_magictriple = IsMagicTripleForm(d, e);

                lock_mutex();
                ++g_d_squares;
                if (de_magictriple)
                {
                    ++g_de_magictriples;
                }
                unlock_mutex();
            }

            // These will likely never happen. I believe they're impossible, but not proven it.
            // A proof would close the Magic Square of Squares problem for 3x3 squares.
            if (f_square && d_square)
            {
                lock_mutex();
                ++g_fd_squares;
                unlock_mutex();
            }
            if (fe_magictriple && de_magictriple)
            {
                lock_mutex();
                ++g_fed_magictriples;
                unlock_mutex();
            }

            ReportResults(j_b, k_b, j_a, k_a, lb, eb, la, ea, F, D, "F", "D", f_square, d_square, fe_magictriple, de_magictriple);

            if ((g_square || c_square) && (f_square || d_square))
            {
                LogMessage("    NOTE: Squares achieved along GEC and FED", "Results.txt");
            }
        }
    #endif
}


// Given j_a, and k_a, which select a Magic Triple for [A, E, I],
// try all possible values for j_b, and k_b, which select a Magic Triple for [B, E, H].
static void TryKa(unsigned long long j_a, unsigned long long k_a)
{
    unsigned long long attempts = 0;
    unsigned long long j_a_sq = j_a * j_a;
    unsigned long long k_a_sq = k_a * k_a;
    unsigned long long l_a = j_a_sq + 2 * j_a * k_a - k_a_sq;
    unsigned long long e_a = j_a_sq + k_a_sq;

    BigInteger big_l_a = BigIntFromULL(l_a);
    BigInteger big_e_a = BigIntFromULL(e_a);

    // Limit j_b to < j_a to avoid the case where j_b == j_a and k_b == k_a.
    // The case were j_b == j_a will be handled below.
    unsigned long long j_b;
    for (j_b = 2; j_b < j_a; ++j_b)
    {
        // See if s_b < 0, where s_b = min_k_b^2 + 2 * j_b * min_k_b - j_b^2.
        unsigned long long j_b_sq = j_b * j_b;
        unsigned long long min_k_b = (j_b * 414213) / 1000000;
        while (min_k_b * (min_k_b + 2 * j_b) < j_b_sq)
        {
            ++min_k_b;
        }

        int k_b_inc;
        if ((j_b & 1) == 0) {
            // If j_b is even, only use odd values for k_b.
            min_k_b |= 1; // Start with odd min_k_b.
            k_b_inc = 2;  // Increment k_b by 2 to test only odd values.
        }
        else {
            k_b_inc = 1;  // If j_b is odd, k_b can be even or odd.
        }

        for (unsigned long long k_b = min_k_b; k_b < j_b; k_b += k_b_inc)
        {
            if (!RelativelyPrime(j_b, k_b)) {
                continue;
            }

            unsigned long long k_b_sq = k_b * k_b;
            unsigned long long l_b = j_b_sq + 2 * j_b * k_b - k_b_sq;
            unsigned long long e_b = j_b_sq + k_b_sq;

            BigInteger big_l_b = BigIntFromULL(l_b);
            BigInteger big_e_b = BigIntFromULL(e_b);

            TryAandB(j_a, k_a, j_b, k_b, big_l_a, big_e_a, big_l_b, big_e_b);
            ++attempts;
        } // Loop on k_b.
    } // Loop on j_b.

    // Handle the case where j_b == j_a.
    j_b = j_a;
    unsigned long long j_b_sq = j_b * j_b;
    unsigned long long min_k_b = (j_b * 414213) / 1000000;
    while (min_k_b * (min_k_b + 2 * j_b) < j_b_sq)
    {
        ++min_k_b;
    }

    for (unsigned long long k_b = min_k_b; k_b < j_b; k_b += 2)
    {
        // Skip the case where k_b == k_a and j_b == j_a.
        if ((k_b == k_a) || (!RelativelyPrime(j_a, k_b))) {
            continue;
        }

        unsigned long long k_b_sq = k_b * k_b;
        unsigned long long l_b = j_a_sq + 2 * j_a * k_b - k_b_sq;
        unsigned long long e_b = j_a_sq + k_b_sq;

        BigInteger big_l_b = BigIntFromULL(l_b);
        BigInteger big_e_b = BigIntFromULL(e_b);

        TryAandB(j_a, k_a, j_a, k_b, big_l_a, big_e_a, big_l_b, big_e_b);
        ++attempts;
    } // Loop on k_b when j_b == j_a.

    lock_mutex();
    g_batch_attempts += attempts;
    g_total_attempts += attempts;
    unlock_mutex();
}


// Arguments passed to threads.
typedef struct thread_data {
    unsigned long long j_a;
    unsigned long long k_a;
} ThreadData;


// Thread function, which calls TryKa for the given j_a and k_a.
#ifdef __linux__
static void* ThreadFunc(void* arg)
#elif _WIN32
static unsigned __stdcall ThreadFunc(void* arg)
#endif
{
    ThreadData* data = (thread_data*) arg;
    TryKa(data->j_a, data->k_a);
    return 0;
}


// Try a given value for j_a, used to select a Magic Triple for [A, E, I].
static void TryJa(unsigned long long j_a)
{
    g_batch_attempts = 0;

    #ifdef __linux__
        pthread_t pthreads[NUM_THREADS] = { 0 };
    #elif _WIN32
        HANDLE threads[NUM_THREADS] = { 0 };
    #endif

    ThreadData args[NUM_THREADS] = { 0 };
    unsigned int threads_used = 0;

    unsigned long long min_k_a = (j_a * 414213) / 1000000;

    // See if s_a < 0, where s_a = min_k_a^2 + 2 * j_a * min_k_a - j_a^2.
    unsigned long long j_a_sq = j_a * j_a;
    while (min_k_a * (min_k_a + 2 * j_a) < j_a_sq)
    {
        ++min_k_a;
    }

    int k_a_inc;
    if ((j_a & 1) == 0) {
        // If j_a is even, only use odd values for k_a.
        min_k_a |= 1; //Start with odd min_k_a.
        k_a_inc = 2;  // Increment k_a by 2 to test only odd values.
    } else {
        k_a_inc = 1;  // If j_a is odd, k_a can be even or odd.
    }

    for (unsigned long long k_a = min_k_a; k_a < j_a; k_a += k_a_inc)
    {
        if (!RelativelyPrime(j_a, k_a))
        {
            continue;
        }

        // Launch a thread to test this j_a and k_a.
        args[threads_used].j_a = j_a;
        args[threads_used].k_a = k_a;

        #ifdef __linux__
            pthread_create(&pthreads[threads_used], NULL, ThreadFunc, &args[threads_used]);
        #elif _WIN32
            threads[threads_used] = (HANDLE)_beginthreadex(NULL, 0, ThreadFunc, &args[threads_used], 0, NULL);
        #endif
        ++threads_used;

        // After NUM_THREADS threads, wait for them to finish before launching more.
        if (threads_used == NUM_THREADS)
        {
            #ifdef __linux__
                for (int i = 0; i <threads_used; i++)
                {
                    pthread_join(pthreads[i], NULL);
                }
            #elif _WIN32
                WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
                for (unsigned int i = 0; i < threads_used; ++i) {
                    if (NULL != threads[i])
                    {
                        CloseHandle(threads[i]);
                        threads[i] = NULL;
                    }
                }
            #endif
            threads_used = 0;
        }
    } // Loop on k_a.

    // Wait for any remaining threads to finish.
    if (threads_used > 0) {
        #ifdef __linux__
            for (unsigned int i = 0; i < threads_used; i++)
            {
                pthread_join(pthreads[i], NULL);
            }
        #elif _WIN32
            WaitForMultipleObjects(threads_used, threads, TRUE, INFINITE);
            for (unsigned int i = 0; i < threads_used; ++i) {
                if (NULL != threads[i])
                {
                    CloseHandle(threads[i]);
                    threads[i] = NULL;
                }
            }
        #endif
        threads_used = 0;
    }
}


/**********************************************************************
 **** Used once - Search for an efficient modulus to quickly       ****
 **** eliminate numbers that are not perfect squares.              ****
 *********************************************************************/


// Find the best modulus for possible_squares
static void trymodulus(void)
{
    static const int LIMIT = 100000;
    double best_ratio = 1.0f;
    bool * squares = (bool *) malloc(sizeof(bool) * LIMIT);
    if (NULL == squares) {
        fprintf(stderr, "Failed to allocate memory for squares[].\n");
        return;
    }

    // Try all moduli from 2 to LIMIT.
    for (int try_mod = 2; try_mod < LIMIT; ++try_mod)
    {
        // Clear the squares array.
        for (int j = 0; j < LIMIT; ++j)
        {
            squares[j] = false;
        }
        // Set squares for this modulus.
        for (int j = 0; j < try_mod; ++j)
        {
            squares[(j * j) % try_mod] = true;
        }
        // Count squares
        int square_count = 0;
        for (int j = 0; j < try_mod; ++j)
        {
            if (squares[j])
            {
                ++square_count;
            }
        }
        double ratio = ((double)square_count) / ((double)try_mod);
        if (ratio < best_ratio)
        {
            best_ratio = ratio;
            fprintf(stdout, "Modulus %i has %i squares (ratio=%0.4f)\n", try_mod, square_count, ratio);
        }
    }

    free(squares);
}


/**********************************************************************
 **** Search for Magic Square Solutions                            ****
 *********************************************************************/


static void InitPossibleSquares()
{
    // Initialize possible_squares.
    for (unsigned int i = 0; i < POSSIBLE_SQUARES_MODULUS; ++i)
    {
        possible_squares[i] = false;
    }
    for (unsigned int i = 0; i < POSSIBLE_SQUARES_MODULUS; ++i)
    {
        possible_squares[(i * i) % POSSIBLE_SQUARES_MODULUS] = true;
    }
}


static void SearchForSolutions(int argc, char** argv)
{
    // Read command-line arguments: starting j_a, prior attempts, prior G^2, prior C^2, prior F^2, prior D^2.
    unsigned long long j_a = 1;
    if (argc > 1)
    {
        j_a = atoi(argv[1]);

        if (argc > 2) {
            g_total_attempts = strtoull(argv[2], NULL, 0);
            if (argc > 4) {
                g_g_squares = strtoull(argv[3], NULL, 0);
                g_c_squares = strtoull(argv[4], NULL, 0);
                #if defined(__CHECK_FED__)
                    if (argc > 6) {
                        g_f_squares = strtoull(argv[5], NULL, 0);
                        g_d_squares = strtoull(argv[6], NULL, 0);
                    }
                #endif
            }
        }
    }

    // Print out start time and parameters.
    char j_a_str[NUM_STR_LEN];
    char total_attempts_str[NUM_STR_LEN];
    char g_g_squares_str[NUM_STR_LEN];
    char g_c_squares_str[NUM_STR_LEN];
    #ifdef __CHECK_FED__
        char g_f_squares_str[NUM_STR_LEN];
        char g_d_squares_str[NUM_STR_LEN];
    #endif

    FormatWithCommas(j_a, j_a_str, NUM_STR_LEN);
    FormatWithCommas(g_total_attempts, total_attempts_str, NUM_STR_LEN);
    FormatWithCommas(g_g_squares, g_g_squares_str, NUM_STR_LEN);
    FormatWithCommas(g_c_squares, g_c_squares_str, NUM_STR_LEN);

    // Format current date and time
    char date_and_time[100];
    time_t now = time(nullptr);
    #ifdef __linux__
        struct tm* timeinfo = localtime(&now);
        strftime(date_and_time, sizeof(date_and_time), "%m-%d %H:%M", timeinfo);
    #elif _WIN32
        struct tm timeinfo;
        localtime_s(&timeinfo, &now);
        strftime(date_and_time, sizeof(date_and_time), "%m-%d %H:%M", &timeinfo);
        InitializeCriticalSection(&g_OutputLock);
    #endif

    #ifdef __CHECK_FED__
        FormatWithCommas(g_f_squares, g_f_squares_str, NUM_STR_LEN);
        FormatWithCommas(g_d_squares, g_d_squares_str, NUM_STR_LEN);
        fprintf(stdout, "%s: Starting at j_a=%s with %s prior attempts, %s G^2, %s C^2, %s F^2, %s D^2.\n",
            date_and_time, j_a_str, total_attempts_str, g_g_squares_str, g_c_squares_str, g_f_squares_str, g_d_squares_str);
    #else
        fprintf(stdout, "%s: Starting at j_a=%s with %s prior attempts, %s G^2, %s C^2.\n",
            date_and_time, j_a_str, total_attempts_str, g_g_squares_str, g_c_squares_str);
    #endif

    // Loop indefinitely, looking for solutions.
    while (1)
    {
        TryJa(j_a);
        ReportProgress(j_a);
        ++j_a;
        WriteResumeScript(j_a);
    }

    #ifdef _WIN32
        DeleteCriticalSection(&g_OutputLock);
    #endif
}

/**********************************************************************
 **** Check solutions.
 **** Read a file with one set of seeds per line (j_a, k_a, j_b, k_a).
 **** Check whether the resulting C, D, F, or G is in Magic Triple form.
 *********************************************************************/

// Tell if the number is in the form a = j^2 + 2jk - k^2.
static bool IsAForm(BigInteger& a)
{
    // See if a is of the form (j^2 + 2jk - k^2).
    bool is_perfect_root = false;
    BigInteger max_j = IntegerRoot(a, &is_perfect_root) + 1;

    for (BigInteger j = max_j; j > 0; --j)
    {
        BigInteger det = j * j * 2 - a;
        if (det.getSign() == BigInteger::negative) {
            break;
        }
        if (IsPerfectSquare(det)) {
            return true;
        }
    }

    return false;
}


static int** ReadSeedFile(const char* filename, size_t* out_rows) {
    #ifdef __linux__
        FILE* fp = fopen(filename, "r");
    #elif _WIN32
        FILE* fp = NULL;
        fopen_s(&fp, filename, "r");
    #endif

    if (!fp) {
        perror("Failed to open file");
        return NULL;
    }

    size_t capacity = 3000;          // initial row capacity
    size_t rows = 0;
    int** array = (int**)malloc(capacity * sizeof(int*));
    if (!array) {
        fclose(fp);
        return NULL;
    }

    while (1) {
        int a, b, c, d;

        #ifdef __linux__
            int result = fscanf(fp, "%d,%d,%d,%d", &a, &b, &c, &d);
        #elif _WIN32
            int result = fscanf_s(fp, "%d,%d,%d,%d", &a, &b, &c, &d);
        #endif

        if (result == EOF)
            break;

        if (result != 4) {
            fprintf(stderr, "Malformed line in file\n");
            fclose(fp);
            return NULL;
        }

        if (rows >= capacity) {
            capacity *= 2;
            int** temp = (int**)realloc(array, capacity * sizeof(int*));
            if (!temp) {
                fclose(fp);
                return NULL;
            }
            array = temp;
        }

        array[rows] = (int *) malloc(4 * sizeof(int));
        if (!array[rows]) {
            fclose(fp);
            return NULL;
        }

        array[rows][0] = a;
        array[rows][1] = b;
        array[rows][2] = c;
        array[rows][3] = d;

        rows++;
    }

    fclose(fp);
    *out_rows = rows;
    return array;
}


static void CheckSolution(int j_a, int k_a, int j_b, int k_b)
{
    BigInteger l_a = j_a * j_a + 2 * j_a * k_a - k_a * k_a;
    BigInteger e_a = j_a * j_a + k_a * k_a;
    BigInteger s_a = k_a * k_a + 2 * k_a * j_a - j_a * j_a;
    BigInteger l_b = j_b * j_b + 2 * j_b * k_b - k_b * k_b;
    BigInteger e_b = j_b * j_b + k_b * k_b;
    BigInteger s_b = k_b * k_b + 2 * k_b * j_b - j_b * j_b;

    // Selected Numbers
    BigInteger A = l_a * e_b;
    BigInteger B = e_a * l_b;
    BigInteger E = e_a * e_b;
    BigInteger H = e_a * s_b;
    BigInteger I = s_a * e_b;
    A *= A;
    B *= B;
    E *= E;
    H *= H;
    I *= I;

    // Calculate Numbers
    BigInteger G = A + B - E;
    BigInteger C = E * 2 - G;
    BigInteger D = E * 3 - A - G;
    BigInteger F = E * 2 - D;

    int squares_found = 0;
    int in_a_form = 0;
    if (IsPerfectSquare(G)) {
        ++squares_found;
        if (IsAForm(G)) {
            ++in_a_form;
        }
    }
    if (IsPerfectSquare(C)) {
        ++squares_found;
        if (IsAForm(C)) {
            ++in_a_form;
        }
    }
    if (IsPerfectSquare(D)) {
        ++squares_found;
        if (IsAForm(D)) {
            ++in_a_form;
        }
    }
    if (IsPerfectSquare(F)) {
        ++squares_found;
        if (IsAForm(F)) {
            ++in_a_form;
        }
    }

    if ((squares_found != 1) || (in_a_form != 1)) {
        printf("%d squares, %d in A Form, found for j_a=%d, k_a=%d, j_b=%d, k_b=%d\n", squares_found, in_a_form, j_a, k_a, j_b, k_b);
    }
}


static void CheckSolutions(int argc, char** argv)
{
    if (argc <= 2) {
        fprintf(stderr, "To check solutions use: -c filename");
        return;
    }

    size_t num_sets = 0;
    int** seed_sets = ReadSeedFile(argv[2], &num_sets);

    for (int i = 0; i < num_sets; ++i)
    {
        CheckSolution(seed_sets[i][0], seed_sets[i][1], seed_sets[i][2], seed_sets[i][3]);
        free(seed_sets[i]);
        seed_sets[i] = NULL;
    }

    free(seed_sets);
    seed_sets = NULL;
}


/**********************************************************************
 **** Main
 **** Search for solutions.
 **** If -c filename is specified, check solutions.
 *********************************************************************/
 

 // Parse arguments, iterate on values for j_a.
int main(int argc, char **argv)
{
    printf("Trying with %d threads.\n", NUM_THREADS);
    //trymodulus(); // Find the most efficient modulus to use in eliminating possible squares.
    InitPossibleSquares();

    if ((argc > 1)
        && ((argv[1][0] == '-') || (argv[1][0] == '/'))
        && ((argv[1][1] == 'c') || (argv[1][1] == 'C'))) {
        CheckSolutions(argc, argv);
    }
    else {
        SearchForSolutions(argc, argv);
    }

    return 0;
}

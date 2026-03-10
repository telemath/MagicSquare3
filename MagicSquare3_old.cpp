// MagicSquare3.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <ctime>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifndef __BIGUNSIGNED_HPP__
#include "BigUnsigned.hpp"
#endif
#ifndef __BIGINTEGERUTILS_HPP__
#include "BigIntegerUtils.hpp"
#endif

// Checks if F and D values for perfect squares and Magic Triple forms.
// Without this, the program will only check G and C values for perfect squares and Magic Triple forms.
#define __CHECK_FED__

// Reports G, C, F and D values that are perfect squares.
#define __REPORT_INDIVIDUAL_SQUARES__

const int MSG_LEN = 3000;
const int NUM_THREADS = 10;
const int NUM_STR_LEN = 25;
const int BIGINT_STR_LEN = 300;

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

static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

static const int POSSIBLE_SQUARES_MODULUS = 65520;
static bool possible_squares [POSSIBLE_SQUARES_MODULUS];

/**********************************************************************
 **** Output functions                                             ****
 *********************************************************************/


 // Output to stdout and to Results.txt.
static void LogMessage(const char* message)
{
    pthread_mutex_lock(&thread_mutex);

    fprintf(stdout, "%s\n", message);

    FILE* fp = fopen("Results.txt", "a");
    if (fp == 0) {
        pthread_mutex_unlock(&thread_mutex);
        return;
    }

    fprintf(fp, "%s\n", message);
    fclose(fp);
    fp = NULL;

    pthread_mutex_unlock(&thread_mutex);
}


static void FormatWithCommas(unsigned long long value, char* output, int output_len)
{
    int cluster0 = value % 1000LL;
    int cluster1 = (value / 1000LL) % 1000LL;
    int cluster2 = (value / 1000000LL) % 1000LL;
    int cluster3 = (value / 1000000000LL) % 1000LL;
    int cluster4 = (value / 1000000000000LL) % 1000LL;
    if (cluster4 > 0) {
        sprintf(output, "%i,%03i,%03i,%03i,%03i", cluster4, cluster3, cluster2, cluster1, cluster0);
    }
    else if (cluster3 > 0) {
        sprintf(output, "%i,%03i,%03i,%03i", cluster3, cluster2, cluster1, cluster0);
    }
    else if (cluster2 > 0) {
        sprintf(output, "%i,%03i,%03i", cluster2, cluster1, cluster0);
    }
    else if (cluster1 > 0) {
        sprintf(output, "%i,%03i", cluster1, cluster0);
    }
    else {
        sprintf(output, "%i", cluster0);
    }
}


static void FormatBigIntWithCommas(BigInteger &i, char * output, int output_len)
{
    std::string num_str = bigIntegerToString(i);
    int len = num_str.length();
    int comma_count = (len - 1) / 3;
    int result_len = len + comma_count;
    
    if (result_len >= output_len) {
        // Output buffer too small, just copy what fits
        strncpy(output, num_str.c_str(), output_len - 1);
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
            FormatWithCommas(j_a, j_a_str, NUM_STR_LEN);
            FormatWithCommas(k_a, k_a_str, NUM_STR_LEN);
            FormatWithCommas(j_b, j_b_str, NUM_STR_LEN);
            FormatWithCommas(k_b, k_b_str, NUM_STR_LEN);
            FormatBigIntWithCommas(l_a, l_a_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(e_a, e_a_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(l_b, l_b_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(e_b, e_b_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(L, L_str, BIGINT_STR_LEN);

            sprintf(message, "    At [Seeds: j_a=%s k_a=%s j_b=%s k_b=%s], [Entries: l_a=%s e_a=%s l_b=%s e_b=%s], %s=%s is a square.",
                    j_a_str, k_a_str, j_b_str, k_b_str, l_a_str, e_a_str, l_b_str, e_b_str, l_name, L_str);
            LogMessage(message);
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
            FormatWithCommas(j_a, j_a_str, NUM_STR_LEN);
            FormatWithCommas(k_a, k_a_str, NUM_STR_LEN);
            FormatWithCommas(j_b, j_b_str, NUM_STR_LEN);
            FormatWithCommas(k_b, k_b_str, NUM_STR_LEN);
            FormatBigIntWithCommas(l_a, l_a_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(e_a, e_a_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(l_b, l_b_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(e_b, e_b_str, BIGINT_STR_LEN);
            FormatBigIntWithCommas(S, S_str, BIGINT_STR_LEN);

            sprintf(message, "    At [Seeds: j_a=%s k_a=%s j_b=%s k_b=%s], [Entries: l_a=%s e_a=%s l_b=%s e_b=%s], %s=%s is a square.",
                    j_a_str, k_a_str, j_b_str, k_b_str, l_a_str, e_a_str, l_b_str, e_b_str, s_name, S_str);
            LogMessage(message);
        }
    #endif

    // I think these conditions are impossible to meet, but have not proven it.
    // A proof would close the Magic Square of Squares problem for 3x3 squares.
    //
    // This section has a lot of sanity checks in case I'm wrong about these statements:
    //     If l and s are squares, they must form a Magic Triple with e.
    //     If l and e form a magic triple, S must also be part of the Magic Triple and therefore a square.
    //     If s and e form a magic triple, L must also be part of the Magic Triple and therefore a square.
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
        sprintf(message_start, "    At [Seeds: j_a=%s k_a=%s j_b=%s k_b=%s], [Entries: l_a=%s e_a=%s l_b=%s e_b=%s]",
                j_a_str, k_a_str, j_b_str, k_b_str, l_a_str, e_a_str, l_b_str, e_b_str);

        if (l_square && s_square)
        {
            sprintf(message, "%s, %s=%s and %s=%s are squares.",
                    message_start, l_name, L_str, s_name, S_str);
            LogMessage(message);

            if (!le_magictriple)
            {
                sprintf(message, "%s, %s=%s and %s=%s are squares, but %s and E=%s are not in Magic Triple form.",
                        message_start, l_name, L_str, s_name, S_str, l_name, E_str);
                LogMessage(message);

                pthread_mutex_lock(&thread_mutex);
                ++g_errors;
                pthread_mutex_unlock(&thread_mutex);
            }
            if (!se_magictriple)
            {
                sprintf(message, "%s, %s=%s and %s=%s are squares, but %s and E=%s are not in Magic Triple form.",
                        message_start, l_name, L_str, s_name, S_str, s_name, E_str);
                LogMessage(message);
                pthread_mutex_lock(&thread_mutex);
                ++g_errors;
                pthread_mutex_unlock(&thread_mutex);
            }
        }
        if (le_magictriple) {
            sprintf(message, "%s, %s=%s and E=%s are in Magic Triple form.",
                    message_start, l_name, L_str, E_str);
            LogMessage(message);

            if (!(l_square && s_square))
            {
                sprintf(message, "%s, %s=%s and E=%s are in Magic Triple form, but %s=%s and %s=%s are not both squares.",
                        message_start, l_name, L_str, E_str, l_name, L_str, s_name, S_str);
                LogMessage(message);
                pthread_mutex_lock(&thread_mutex);
                ++g_errors;
                pthread_mutex_unlock(&thread_mutex);
            }
        }
        if (se_magictriple)
        {
            sprintf(message, "%s, %s=%s and E=%s are in Magic Triple form.",
                    message_start, s_name, S_str, E_str);
            LogMessage(message);

            if (!(l_square && s_square)) {
                sprintf(message, "%s, %s=%s and E=%s are in Magic Triple form, but %s=%s and %s=%s are not both squares.",
                    message_start, s_name, S_str, E_str, l_name, L_str, s_name, S_str);
                LogMessage(message);
                pthread_mutex_lock(&thread_mutex);
                ++g_errors;
                pthread_mutex_unlock(&thread_mutex);
            }
        }
    }
}


static void ReportProgress(int j_a)
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
    struct tm *timeinfo = localtime(&now);
    strftime(date_and_time, sizeof(date_and_time), "%m-%d %H:%M", timeinfo);
    
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

        sprintf(message,
                "%s: j_a=%s, %s tried, %s total, "
                "Squares: %s G, %s C, %s F, %s D, %s GC, %s FD, "
                "Triples: %s GE, %s CE, %s GEC, %s FE, %s DE, %s FED, "
                "%s errs.",
                date_and_time, j_a_str, batch_attempts_str, total_attempts_str,
                g_g_squares_str, g_c_squares_str, g_f_squares_str, g_d_squares_str, g_gc_squares_str, g_fd_squares_str,
                g_ge_magictriples_str, g_ce_magictriples_str, g_gec_magictriples_str, g_fe_magictriples_str, g_de_magictriples_str, g_fed_magictriples_str,
                g_errors_str);
    #else
        sprintf(message,
                "j_a=%s, %s attempts, %s total attempts, "
                "Squares: %s G, %s C, %s GC, "
                "Magic Triples: %s GE, %s CE, %s GEC, "
                "%s errors.",
                j_a_str, batch_attempts_str, total_attempts_str,
                g_g_squares_str, g_c_squares_str, g_gc_squares_str,
                g_ge_magictriples_str, g_ce_magictriples_str, g_gec_magictriples_str,
                g_errors_str);
    #endif
    LogMessage(message);
}


/**********************************************************************
 **** Calculation functions                                        ****
 *********************************************************************/


static BigInteger BigIntFromULL(unsigned long long x)
{
    BigInteger result = (uint32_t) (x >> 32);
    result *= 0x10000;
    result *= 0x10000;
    result += (uint32_t)(x & 0xFFFFFFFF);
    return result;
}


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


static BigInteger IntegerRoot(BigInteger &n)
{
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

    return ((prev_root == root) || (root * root == n)) ? root : prev_root;
}


static bool IsPerfectSquare(BigInteger &n)
{
    // Negative numbers can not be perfect squares.
    if (n.getSign() == BigInteger::negative)
    {
        return false;
    }

    // Check the last four digits mod 10000 to quickly rule out non-squares.
    int last_digits = (n % POSSIBLE_SQUARES_MODULUS).toInt();
    if (!possible_squares[last_digits])
    {
        return false;
    }

    // Do it the hard way.
    BigInteger root = IntegerRoot(n);
    return (root * root == n);
}


static bool IsMagicTripleForm(BigInteger &a, BigInteger &e)
{
    // See if a is of the form (m^2 + 2mx - x^2) and e is of the form (m^2 + x^2).
    BigInteger half_e = e / 2;
    BigInteger quarter_e = half_e / 2;
    BigInteger max_j = IntegerRoot(half_e) + 1;
    for (BigInteger j = 1; j <= max_j; ++j)
    {
        BigInteger j_sq = j * j;
        BigInteger k_sq = e - j_sq;
        if (k_sq.getSign() != BigInteger::positive)
        {
            break;
        }
        BigInteger k = IntegerRoot(k_sq);
        if (k * k == k_sq) // If true, e = m^2 + x^2.
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
        g_square = IsPerfectSquare(G);
        c_square = IsPerfectSquare(C);
        bool ge_magictriple = false;
        bool ce_magictriple = false;

        if (g_square)
        {
            BigInteger g = IntegerRoot(G);
            ge_magictriple = IsMagicTripleForm(g, e);

            pthread_mutex_lock(&thread_mutex);
            ++g_g_squares;
            if (ge_magictriple)
            {
                ++g_ge_magictriples;
            }
            pthread_mutex_unlock(&thread_mutex);
        }

        if (c_square)
        {
            BigInteger c = IntegerRoot(C);
            ce_magictriple = IsMagicTripleForm(c, e);

            pthread_mutex_lock(&thread_mutex);
            ++g_c_squares;
            if (ce_magictriple)
            {
                ++g_ce_magictriples;
            }
            pthread_mutex_unlock(&thread_mutex);
        }

        // These will likely never happen. I believe they're impossible, but not proven it.
        // A proof would close the Magic Square of Squares problem for 3x3 squares.
        if (g_square && c_square)
        {
            pthread_mutex_lock(&thread_mutex);
            ++g_gc_squares;
            pthread_mutex_unlock(&thread_mutex);
        }
        if (ge_magictriple && ce_magictriple)
        {
            pthread_mutex_lock(&thread_mutex);
            ++g_gec_magictriples;
            pthread_mutex_unlock(&thread_mutex);
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
            bool f_square = IsPerfectSquare(F);
            bool d_square = IsPerfectSquare(D);
            bool fe_magictriple = false;
            bool de_magictriple = false;

            if (f_square)
            {
                BigInteger f = IntegerRoot(F);
                fe_magictriple = IsMagicTripleForm(f, e);

                pthread_mutex_lock(&thread_mutex);
                ++g_f_squares;
                if (fe_magictriple)
                {
                    ++g_fe_magictriples;
                }
                pthread_mutex_unlock(&thread_mutex);
            }

            if (d_square)
            {
                BigInteger d = IntegerRoot(D);
                de_magictriple = IsMagicTripleForm(d, e);

                pthread_mutex_lock(&thread_mutex);
                ++g_d_squares;
                if (de_magictriple)
                {
                    ++g_de_magictriples;
                }
                pthread_mutex_unlock(&thread_mutex);
            }

            // These will likely never happen. I believe they're impossible, but not proven it.
            // A proof would close the Magic Square of Squares problem for 3x3 squares.
            if (f_square && d_square)
            {
                pthread_mutex_lock(&thread_mutex);
                ++g_fd_squares;
                pthread_mutex_unlock(&thread_mutex);
            }
            if (fe_magictriple && de_magictriple)
            {
                pthread_mutex_lock(&thread_mutex);
                ++g_fed_magictriples;
                pthread_mutex_unlock(&thread_mutex);
            }

            ReportResults(j_a, k_a, j_b, k_b, la, ea, lb, eb, F, D, "F", "D", f_square, d_square, fe_magictriple, de_magictriple);
            
            if ((g_square || c_square) && (f_square || d_square))
            {
                LogMessage("    NOTE: Squares achieved along GEC and FED");
            }
        }

        // A and B are not interchangeable, so try swapping A and B.
        D = E3 - B - G;
        F = E3 - D - E;

        if ((D.getSign() == BigInteger::positive) && (F.getSign() == BigInteger::positive))
        {
            bool f_square = IsPerfectSquare(F);
            bool d_square = IsPerfectSquare(D);
            bool fe_magictriple = false;
            bool de_magictriple = false;

            if (f_square)
            {
                BigInteger f = IntegerRoot(F);
                fe_magictriple = IsMagicTripleForm(f, e);

                pthread_mutex_lock(&thread_mutex);
                ++g_f_squares;
                if (fe_magictriple)
                {
                    ++g_fe_magictriples;
                }
                pthread_mutex_unlock(&thread_mutex);
            }

            if (d_square)
            {
                BigInteger d = IntegerRoot(D);
                de_magictriple = IsMagicTripleForm(d, e);

                pthread_mutex_lock(&thread_mutex);
                ++g_d_squares;
                if (de_magictriple)
                {
                    ++g_de_magictriples;
                }
                pthread_mutex_unlock(&thread_mutex);
            }

            // These will likely never happen. I believe they're impossible, but not proven it.
            // A proof would close the Magic Square of Squares problem for 3x3 squares.
            if (f_square && d_square)
            {
                pthread_mutex_lock(&thread_mutex);
                ++g_fd_squares;
                pthread_mutex_unlock(&thread_mutex);
            }
            if (fe_magictriple && de_magictriple)
            {
                pthread_mutex_lock(&thread_mutex);
                ++g_fed_magictriples;
                pthread_mutex_unlock(&thread_mutex);
            }

            ReportResults(j_b, k_b, j_a, k_a, lb, eb, la, ea, F, D, "F", "D", f_square, d_square, fe_magictriple, de_magictriple);

            if ((g_square || c_square) && (f_square || d_square))
            {
                LogMessage("    NOTE: Squares achieved along GEC and FED");
            }
        }
    #endif
}


static void TryJa(int start)
{
    unsigned long long j_a = start;
    unsigned long long attempts = 0;
    unsigned long long min_k_a = (j_a * 414213) / 1000000;

    // See if s_a < 0, where s_a = min_k_a^2 + 2 * j_a * min_k_a - j_a^2.
    unsigned long long j_a_sq = j_a * j_a;
    while (min_k_a * (min_k_a + 2 * j_a) < j_a_sq)
    {
        ++min_k_a;
    }

    // j_a and k_a cannot both be odd or both be even.  If they are, the resulting
    // l, e, and s are all divisible by 2 and can be reduced to a previous case.
    unsigned long long k_a = min_k_a;
    if ((k_a & 1) == (j_a & 1)) {
        ++k_a;
    }

    for (; k_a < j_a; k_a += 2)
    {
        if (!RelativelyPrime(j_a, k_a)) {
            continue;
        }

        unsigned long long k_a_sq = k_a * k_a;
        unsigned long long l_a = j_a_sq + 2 * j_a * k_a - k_a_sq;
        unsigned long long e_a = j_a_sq + k_a_sq;

        BigInteger big_l_a = BigIntFromULL(l_a);
        BigInteger big_e_a = BigIntFromULL(e_a);

        unsigned long long min_k_b = (j_a * 414213) / 1000000;

        // Limit j_b to < j_a to avoid the case where j_b == j_a and k_b == k_a.
        // The case were j_b == j_a will be handled below.
        for (unsigned long long j_b = 2; j_b < j_a; ++j_b)
        {
            // See if s_b < 0, where s_b = min_k_b^2 + 2 * j_b * min_k_b - j_b^2.
            unsigned long long j_b_sq = j_b * j_b;
            while (min_k_b * (min_k_b + 2 * j_b) < j_b_sq)
            {
                ++min_k_b;
            }

            // j_b and k_b cannot both be odd or both be even.
            // If they are, the resulting l, e, and s are all divisible by 2.
            unsigned long long k_b = min_k_b;
            if ((k_b & 1) == (j_b & 1)) {
                ++k_b;
            }

            for (; k_b < j_b; k_b += 2)
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
        for (unsigned long long k_b = min_k_a; k_b < j_a; k_b += 2)
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

    } // Loop on k_a.

    pthread_mutex_lock(&thread_mutex);
    g_batch_attempts += attempts;
    g_total_attempts += attempts;
    pthread_mutex_unlock(&thread_mutex);
}


/**********************************************************************
 **** Thread and Main functions                                    ****
 *********************************************************************/


static void *ThreadFunc(void *arg)
{
    int n = *(int*)arg;
    TryJa(n);
    return 0;
}

void trymodulos()
{
    static const int LIMIT = 100000;
    bool squares[LIMIT];
    double best_ratio = 1.0f;

    // Try all moduli from 2 to LIMIT.
    for (int try_mod = 2; try_mod <= LIMIT; ++try_mod)
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
            printf("Modulus %i has %i squares (ratio=%0.4f)\n", try_mod, square_count, ratio);
        }
    }
}

int main(int argc, char **argv)
{
    //trymodulos();

    // Initialize possible_squares.
    for (int i = 0; i < POSSIBLE_SQUARES_MODULUS; ++i)
    {
        possible_squares[i] = false;
    }
    for (int i = 0; i < POSSIBLE_SQUARES_MODULUS; ++i)
    {
        // If a number ends in these four digits, it could be a perfect square.
        possible_squares[(i * i) % POSSIBLE_SQUARES_MODULUS] = true;
    }

    possible_squares [POSSIBLE_SQUARES_MODULUS];

    pthread_t pthreads[10];
    int args[NUM_THREADS];
    char j_a_str[NUM_STR_LEN];
    char total_attempts_str[NUM_STR_LEN];
    char g_g_squares_str[NUM_STR_LEN];
    char g_c_squares_str[NUM_STR_LEN];
    #ifdef __CHECK_FED__
        char g_f_squares_str[NUM_STR_LEN];
        char g_d_squares_str[NUM_STR_LEN];
    #endif

    // Read command-line arguments: starting j_a, prior attempts, prior G^2, prior C^2, prior F^2, prior D^2.
    int start = 1;
    if (argc > 1)
    {
        start = atoi(argv[1]);

        if (argc > 2) {
            g_total_attempts = strtoull(argv[2], NULL, 0);
            if (argc  > 4) {
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

    FormatWithCommas(start, j_a_str, NUM_STR_LEN);
    FormatWithCommas(g_total_attempts, total_attempts_str, NUM_STR_LEN);
    FormatWithCommas(g_g_squares, g_g_squares_str, NUM_STR_LEN);
    FormatWithCommas(g_c_squares, g_c_squares_str, NUM_STR_LEN);

    // Format current date and time
    char date_and_time[100];
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    strftime(date_and_time, sizeof(date_and_time), "%m-%d %H:%M", timeinfo);

    #ifdef __CHECK_FED__
        FormatWithCommas(g_f_squares, g_f_squares_str, NUM_STR_LEN);
        FormatWithCommas(g_d_squares, g_d_squares_str, NUM_STR_LEN);
        printf("%s: Starting at j_a=%s with %s prior attempts, %s G^2, %s C^2, %s F^2, %s D^2.\n",
               date_and_time, j_a_str, total_attempts_str, g_g_squares_str, g_c_squares_str, g_f_squares_str, g_d_squares_str);
    #else
        printf("%s: Starting at j_a=%s with %s prior attempts, %s G^2, %s C^2.\n",
               date_and_time, j_a_str, total_attempts_str, g_g_squares_str, g_c_squares_str);
    #endif

    // Loop indefinitely, looking for solutions.
    while (1)
    {
        g_batch_attempts = 0;
        for (int i = 0; i < NUM_THREADS; i++)
        {
            args[i] = start + i;
            pthread_create(&pthreads[i], NULL, ThreadFunc, &args[i]);
        }

        for (int i = 0; i < NUM_THREADS; i++)
        {
            pthread_join(pthreads[i], NULL);
        }

        ReportProgress(start + NUM_THREADS - 1);
        start += NUM_THREADS;
    }

    return 0;
}

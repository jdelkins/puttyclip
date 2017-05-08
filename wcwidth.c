/*
 * This is an implementation of wcwidth() and wcswidth() (defined in
 * IEEE Std 1002.1-2001) for Unicode.
 *
 * http://www.opengroup.org/onlinepubs/007904975/functions/wcwidth.html
 * http://www.opengroup.org/onlinepubs/007904975/functions/wcswidth.html
 *
 * In fixed-width output devices, Latin characters all occupy a single
 * "cell" position of equal width, whereas ideographic CJK characters
 * occupy two such cells. Interoperability between terminal-line
 * applications and (teletype-style) character terminals using the
 * UTF-8 encoding requires agreement on which character should advance
 * the cursor by how many cell positions. No established formal
 * standards exist at present on which Unicode character shall occupy
 * how many cell positions on character terminals. These routines are
 * a first attempt of defining such behavior based on simple rules
 * applied to data provided by the Unicode Consortium.
 *
 * For some graphical characters, the Unicode standard explicitly
 * defines a character-cell width via the definition of the East Asian
 * FullWidth (F), Wide (W), Half-width (H), and Narrow (Na) classes.
 * In all these cases, there is no ambiguity about which width a
 * terminal shall use. For characters in the East Asian Ambiguous (A)
 * class, the width choice depends purely on a preference of backward
 * compatibility with either historic CJK or Western practice.
 * Choosing single-width for these characters is easy to justify as
 * the appropriate long-term solution, as the CJK practice of
 * displaying these characters as double-width comes from historic
 * implementation simplicity (8-bit encoded characters were displayed
 * single-width and 16-bit ones double-width, even for Greek,
 * Cyrillic, etc.) and not any typographic considerations.
 *
 * Much less clear is the choice of width for the Not East Asian
 * (Neutral) class. Existing practice does not dictate a width for any
 * of these characters. It would nevertheless make sense
 * typographically to allocate two character cells to characters such
 * as for instance EM SPACE or VOLUME INTEGRAL, which cannot be
 * represented adequately with a single-width glyph. The following
 * routines at present merely assign a single-cell width to all
 * neutral characters, in the interest of simplicity. This is not
 * entirely satisfactory and should be reconsidered before
 * establishing a formal standard in this area. At the moment, the
 * decision which Not East Asian (Neutral) characters should be
 * represented by double-width glyphs cannot yet be answered by
 * applying a simple rule from the Unicode database content. Setting
 * up a proper standard for the behavior of UTF-8 character terminals
 * will require a careful analysis not only of each Unicode character,
 * but also of each presentation form, something the author of these
 * routines has avoided to do so far.
 *
 * http://www.unicode.org/unicode/reports/tr11/
 *
 * Markus Kuhn -- 2007-05-26 (Unicode 5.0)
 *
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted. The author
 * disclaims all warranties with regard to this software.
 *
 * Latest version: http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
 */

#include <wchar.h>

#include "putty.h" /* for prototypes */

struct interval {
  unsigned int first;
  unsigned int last;
};

/* auxiliary function for binary search in interval table */
static int bisearch(unsigned int ucs, const struct interval *table, int max) {
  int min = 0;
  int mid;

  if (ucs < table[0].first || ucs > table[max].last)
    return 0;
  while (max >= min) {
    mid = (min + max) / 2;
    if (ucs > table[mid].last)
      min = mid + 1;
    else if (ucs < table[mid].first)
      max = mid - 1;
    else
      return 1;
  }

  return 0;
}


/* The following two functions define the column width of an ISO 10646
 * character as follows:
 *
 *    - The null character (U+0000) has a column width of 0.
 *
 *    - Other C0/C1 control characters and DEL will lead to a return
 *      value of -1.
 *
 *    - Non-spacing and enclosing combining characters (general
 *      category code Mn or Me in the Unicode database) have a
 *      column width of 0.
 *
 *    - SOFT HYPHEN (U+00AD) has a column width of 1.
 *
 *    - Other format characters (general category code Cf in the Unicode
 *      database) and ZERO WIDTH SPACE (U+200B) have a column width of 0.
 *
 *    - Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
 *      have a column width of 0.
 *
 *    - Spacing characters in the East Asian Wide (W) or East Asian
 *      Full-width (F) category as defined in Unicode Technical
 *      Report #11 have a column width of 2.
 *
 *    - All remaining characters (including all printable
 *      ISO 8859-1 and WGL4 characters, Unicode control characters,
 *      etc.) have a column width of 1.
 *
 * This implementation assumes that wchar_t characters are encoded
 * in ISO 10646.
 */

int mk_wcwidth(unsigned int ucs)
{
  /* sorted list of non-overlapping intervals of non-spacing characters */
  /* All Mn, Me and Cf characters from version 8.0.0 of
     http://www.unicode.org/Public/UNIDATA/extracted/DerivedGeneralCategory.txt
   */

  static const struct interval combining[] = {
    {0x00ad, 0x00ad},	/* SOFT HYPHEN to SOFT HYPHEN */
    {0x0300, 0x036f},	/* COMBINING GRAVE ACCENT to COMBINING LATIN SMALL LETTER X */
    {0x0483, 0x0489},	/* COMBINING CYRILLIC TITLO to COMBINING CYRILLIC MILLIONS SIGN */
    {0x0591, 0x05bd},	/* HEBREW ACCENT ETNAHTA to HEBREW POINT METEG */
    {0x05bf, 0x05bf},	/* HEBREW POINT RAFE to HEBREW POINT RAFE */
    {0x05c1, 0x05c2},	/* HEBREW POINT SHIN DOT to HEBREW POINT SIN DOT */
    {0x05c4, 0x05c5},	/* HEBREW MARK UPPER DOT to HEBREW MARK LOWER DOT */
    {0x05c7, 0x05c7},	/* HEBREW POINT QAMATS QATAN to HEBREW POINT QAMATS QATAN */
    {0x0600, 0x0605},	/* ARABIC NUMBER SIGN to ARABIC NUMBER MARK ABOVE */
    {0x0610, 0x061a},	/* ARABIC SIGN SALLALLAHOU ALAYHE WASSALLAM to ARABIC SMALL KASRA */
    {0x061c, 0x061c},	/* ARABIC LETTER MARK to ARABIC LETTER MARK */
    {0x064b, 0x065f},	/* ARABIC FATHATAN to ARABIC WAVY HAMZA BELOW */
    {0x0670, 0x0670},	/* ARABIC LETTER SUPERSCRIPT ALEF to ARABIC LETTER SUPERSCRIPT ALEF */
    {0x06d6, 0x06dd},	/* ARABIC SMALL HIGH LIGATURE SAD WITH LAM WITH ALEF MAKSURA to ARABIC END OF AYAH */
    {0x06df, 0x06e4},	/* ARABIC SMALL HIGH ROUNDED ZERO to ARABIC SMALL HIGH MADDA */
    {0x06e7, 0x06e8},	/* ARABIC SMALL HIGH YEH to ARABIC SMALL HIGH NOON */
    {0x06ea, 0x06ed},	/* ARABIC EMPTY CENTRE LOW STOP to ARABIC SMALL LOW MEEM */
    {0x070f, 0x070f},	/* SYRIAC ABBREVIATION MARK to SYRIAC ABBREVIATION MARK */
    {0x0711, 0x0711},	/* SYRIAC LETTER SUPERSCRIPT ALAPH to SYRIAC LETTER SUPERSCRIPT ALAPH */
    {0x0730, 0x074a},	/* SYRIAC PTHAHA ABOVE to SYRIAC BARREKH */
    {0x07a6, 0x07b0},	/* THAANA ABAFILI to THAANA SUKUN */
    {0x07eb, 0x07f3},	/* NKO COMBINING SHORT HIGH TONE to NKO COMBINING DOUBLE DOT ABOVE */
    {0x0816, 0x0819},	/* SAMARITAN MARK IN to SAMARITAN MARK DAGESH */
    {0x081b, 0x0823},	/* SAMARITAN MARK EPENTHETIC YUT to SAMARITAN VOWEL SIGN A */
    {0x0825, 0x0827},	/* SAMARITAN VOWEL SIGN SHORT A to SAMARITAN VOWEL SIGN U */
    {0x0829, 0x082d},	/* SAMARITAN VOWEL SIGN LONG I to SAMARITAN MARK NEQUDAA */
    {0x0859, 0x085b},	/* MANDAIC AFFRICATION MARK to MANDAIC GEMINATION MARK */
    {0x08e3, 0x0902},	/* ARABIC TURNED DAMMA BELOW to DEVANAGARI SIGN ANUSVARA */
    {0x093a, 0x093a},	/* DEVANAGARI VOWEL SIGN OE to DEVANAGARI VOWEL SIGN OE */
    {0x093c, 0x093c},	/* DEVANAGARI SIGN NUKTA to DEVANAGARI SIGN NUKTA */
    {0x0941, 0x0948},	/* DEVANAGARI VOWEL SIGN U to DEVANAGARI VOWEL SIGN AI */
    {0x094d, 0x094d},	/* DEVANAGARI SIGN VIRAMA to DEVANAGARI SIGN VIRAMA */
    {0x0951, 0x0957},	/* DEVANAGARI STRESS SIGN UDATTA to DEVANAGARI VOWEL SIGN UUE */
    {0x0962, 0x0963},	/* DEVANAGARI VOWEL SIGN VOCALIC L to DEVANAGARI VOWEL SIGN VOCALIC LL */
    {0x0981, 0x0981},	/* BENGALI SIGN CANDRABINDU to BENGALI SIGN CANDRABINDU */
    {0x09bc, 0x09bc},	/* BENGALI SIGN NUKTA to BENGALI SIGN NUKTA */
    {0x09c1, 0x09c4},	/* BENGALI VOWEL SIGN U to BENGALI VOWEL SIGN VOCALIC RR */
    {0x09cd, 0x09cd},	/* BENGALI SIGN VIRAMA to BENGALI SIGN VIRAMA */
    {0x09e2, 0x09e3},	/* BENGALI VOWEL SIGN VOCALIC L to BENGALI VOWEL SIGN VOCALIC LL */
    {0x0a01, 0x0a02},	/* GURMUKHI SIGN ADAK BINDI to GURMUKHI SIGN BINDI */
    {0x0a3c, 0x0a3c},	/* GURMUKHI SIGN NUKTA to GURMUKHI SIGN NUKTA */
    {0x0a41, 0x0a42},	/* GURMUKHI VOWEL SIGN U to GURMUKHI VOWEL SIGN UU */
    {0x0a47, 0x0a48},	/* GURMUKHI VOWEL SIGN EE to GURMUKHI VOWEL SIGN AI */
    {0x0a4b, 0x0a4d},	/* GURMUKHI VOWEL SIGN OO to GURMUKHI SIGN VIRAMA */
    {0x0a51, 0x0a51},	/* GURMUKHI SIGN UDAAT to GURMUKHI SIGN UDAAT */
    {0x0a70, 0x0a71},	/* GURMUKHI TIPPI to GURMUKHI ADDAK */
    {0x0a75, 0x0a75},	/* GURMUKHI SIGN YAKASH to GURMUKHI SIGN YAKASH */
    {0x0a81, 0x0a82},	/* GUJARATI SIGN CANDRABINDU to GUJARATI SIGN ANUSVARA */
    {0x0abc, 0x0abc},	/* GUJARATI SIGN NUKTA to GUJARATI SIGN NUKTA */
    {0x0ac1, 0x0ac5},	/* GUJARATI VOWEL SIGN U to GUJARATI VOWEL SIGN CANDRA E */
    {0x0ac7, 0x0ac8},	/* GUJARATI VOWEL SIGN E to GUJARATI VOWEL SIGN AI */
    {0x0acd, 0x0acd},	/* GUJARATI SIGN VIRAMA to GUJARATI SIGN VIRAMA */
    {0x0ae2, 0x0ae3},	/* GUJARATI VOWEL SIGN VOCALIC L to GUJARATI VOWEL SIGN VOCALIC LL */
    {0x0b01, 0x0b01},	/* ORIYA SIGN CANDRABINDU to ORIYA SIGN CANDRABINDU */
    {0x0b3c, 0x0b3c},	/* ORIYA SIGN NUKTA to ORIYA SIGN NUKTA */
    {0x0b3f, 0x0b3f},	/* ORIYA VOWEL SIGN I to ORIYA VOWEL SIGN I */
    {0x0b41, 0x0b44},	/* ORIYA VOWEL SIGN U to ORIYA VOWEL SIGN VOCALIC RR */
    {0x0b4d, 0x0b4d},	/* ORIYA SIGN VIRAMA to ORIYA SIGN VIRAMA */
    {0x0b56, 0x0b56},	/* ORIYA AI LENGTH MARK to ORIYA AI LENGTH MARK */
    {0x0b62, 0x0b63},	/* ORIYA VOWEL SIGN VOCALIC L to ORIYA VOWEL SIGN VOCALIC LL */
    {0x0b82, 0x0b82},	/* TAMIL SIGN ANUSVARA to TAMIL SIGN ANUSVARA */
    {0x0bc0, 0x0bc0},	/* TAMIL VOWEL SIGN II to TAMIL VOWEL SIGN II */
    {0x0bcd, 0x0bcd},	/* TAMIL SIGN VIRAMA to TAMIL SIGN VIRAMA */
    {0x0c00, 0x0c00},	/* TELUGU SIGN COMBINING CANDRABINDU ABOVE to TELUGU SIGN COMBINING CANDRABINDU ABOVE */
    {0x0c3e, 0x0c40},	/* TELUGU VOWEL SIGN AA to TELUGU VOWEL SIGN II */
    {0x0c46, 0x0c48},	/* TELUGU VOWEL SIGN E to TELUGU VOWEL SIGN AI */
    {0x0c4a, 0x0c4d},	/* TELUGU VOWEL SIGN O to TELUGU SIGN VIRAMA */
    {0x0c55, 0x0c56},	/* TELUGU LENGTH MARK to TELUGU AI LENGTH MARK */
    {0x0c62, 0x0c63},	/* TELUGU VOWEL SIGN VOCALIC L to TELUGU VOWEL SIGN VOCALIC LL */
    {0x0c81, 0x0c81},	/* KANNADA SIGN CANDRABINDU to KANNADA SIGN CANDRABINDU */
    {0x0cbc, 0x0cbc},	/* KANNADA SIGN NUKTA to KANNADA SIGN NUKTA */
    {0x0cbf, 0x0cbf},	/* KANNADA VOWEL SIGN I to KANNADA VOWEL SIGN I */
    {0x0cc6, 0x0cc6},	/* KANNADA VOWEL SIGN E to KANNADA VOWEL SIGN E */
    {0x0ccc, 0x0ccd},	/* KANNADA VOWEL SIGN AU to KANNADA SIGN VIRAMA */
    {0x0ce2, 0x0ce3},	/* KANNADA VOWEL SIGN VOCALIC L to KANNADA VOWEL SIGN VOCALIC LL */
    {0x0d01, 0x0d01},	/* MALAYALAM SIGN CANDRABINDU to MALAYALAM SIGN CANDRABINDU */
    {0x0d41, 0x0d44},	/* MALAYALAM VOWEL SIGN U to MALAYALAM VOWEL SIGN VOCALIC RR */
    {0x0d4d, 0x0d4d},	/* MALAYALAM SIGN VIRAMA to MALAYALAM SIGN VIRAMA */
    {0x0d62, 0x0d63},	/* MALAYALAM VOWEL SIGN VOCALIC L to MALAYALAM VOWEL SIGN VOCALIC LL */
    {0x0dca, 0x0dca},	/* SINHALA SIGN AL-LAKUNA to SINHALA SIGN AL-LAKUNA */
    {0x0dd2, 0x0dd4},	/* SINHALA VOWEL SIGN KETTI IS-PILLA to SINHALA VOWEL SIGN KETTI PAA-PILLA */
    {0x0dd6, 0x0dd6},	/* SINHALA VOWEL SIGN DIGA PAA-PILLA to SINHALA VOWEL SIGN DIGA PAA-PILLA */
    {0x0e31, 0x0e31},	/* THAI CHARACTER MAI HAN-AKAT to THAI CHARACTER MAI HAN-AKAT */
    {0x0e34, 0x0e3a},	/* THAI CHARACTER SARA I to THAI CHARACTER PHINTHU */
    {0x0e47, 0x0e4e},	/* THAI CHARACTER MAITAIKHU to THAI CHARACTER YAMAKKAN */
    {0x0eb1, 0x0eb1},	/* LAO VOWEL SIGN MAI KAN to LAO VOWEL SIGN MAI KAN */
    {0x0eb4, 0x0eb9},	/* LAO VOWEL SIGN I to LAO VOWEL SIGN UU */
    {0x0ebb, 0x0ebc},	/* LAO VOWEL SIGN MAI KON to LAO SEMIVOWEL SIGN LO */
    {0x0ec8, 0x0ecd},	/* LAO TONE MAI EK to LAO NIGGAHITA */
    {0x0f18, 0x0f19},	/* TIBETAN ASTROLOGICAL SIGN -KHYUD PA to TIBETAN ASTROLOGICAL SIGN SDONG TSHUGS */
    {0x0f35, 0x0f35},	/* TIBETAN MARK NGAS BZUNG NYI ZLA to TIBETAN MARK NGAS BZUNG NYI ZLA */
    {0x0f37, 0x0f37},	/* TIBETAN MARK NGAS BZUNG SGOR RTAGS to TIBETAN MARK NGAS BZUNG SGOR RTAGS */
    {0x0f39, 0x0f39},	/* TIBETAN MARK TSA -PHRU to TIBETAN MARK TSA -PHRU */
    {0x0f71, 0x0f7e},	/* TIBETAN VOWEL SIGN AA to TIBETAN SIGN RJES SU NGA RO */
    {0x0f80, 0x0f84},	/* TIBETAN VOWEL SIGN REVERSED I to TIBETAN MARK HALANTA */
    {0x0f86, 0x0f87},	/* TIBETAN SIGN LCI RTAGS to TIBETAN SIGN YANG RTAGS */
    {0x0f8d, 0x0f97},	/* TIBETAN SUBJOINED SIGN LCE TSA CAN to TIBETAN SUBJOINED LETTER JA */
    {0x0f99, 0x0fbc},	/* TIBETAN SUBJOINED LETTER NYA to TIBETAN SUBJOINED LETTER FIXED-FORM RA */
    {0x0fc6, 0x0fc6},	/* TIBETAN SYMBOL PADMA GDAN to TIBETAN SYMBOL PADMA GDAN */
    {0x102d, 0x1030},	/* MYANMAR VOWEL SIGN I to MYANMAR VOWEL SIGN UU */
    {0x1032, 0x1037},	/* MYANMAR VOWEL SIGN AI to MYANMAR SIGN DOT BELOW */
    {0x1039, 0x103a},	/* MYANMAR SIGN VIRAMA to MYANMAR SIGN ASAT */
    {0x103d, 0x103e},	/* MYANMAR CONSONANT SIGN MEDIAL WA to MYANMAR CONSONANT SIGN MEDIAL HA */
    {0x1058, 0x1059},	/* MYANMAR VOWEL SIGN VOCALIC L to MYANMAR VOWEL SIGN VOCALIC LL */
    {0x105e, 0x1060},	/* MYANMAR CONSONANT SIGN MON MEDIAL NA to MYANMAR CONSONANT SIGN MON MEDIAL LA */
    {0x1071, 0x1074},	/* MYANMAR VOWEL SIGN GEBA KAREN I to MYANMAR VOWEL SIGN KAYAH EE */
    {0x1082, 0x1082},	/* MYANMAR CONSONANT SIGN SHAN MEDIAL WA to MYANMAR CONSONANT SIGN SHAN MEDIAL WA */
    {0x1085, 0x1086},	/* MYANMAR VOWEL SIGN SHAN E ABOVE to MYANMAR VOWEL SIGN SHAN FINAL Y */
    {0x108d, 0x108d},	/* MYANMAR SIGN SHAN COUNCIL EMPHATIC TONE to MYANMAR SIGN SHAN COUNCIL EMPHATIC TONE */
    {0x109d, 0x109d},	/* MYANMAR VOWEL SIGN AITON AI to MYANMAR VOWEL SIGN AITON AI */
    {0x135d, 0x135f},	/* ETHIOPIC COMBINING GEMINATION AND VOWEL LENGTH MARK to ETHIOPIC COMBINING GEMINATION MARK */
    {0x1712, 0x1714},	/* TAGALOG VOWEL SIGN I to TAGALOG SIGN VIRAMA */
    {0x1732, 0x1734},	/* HANUNOO VOWEL SIGN I to HANUNOO SIGN PAMUDPOD */
    {0x1752, 0x1753},	/* BUHID VOWEL SIGN I to BUHID VOWEL SIGN U */
    {0x1772, 0x1773},	/* TAGBANWA VOWEL SIGN I to TAGBANWA VOWEL SIGN U */
    {0x17b4, 0x17b5},	/* KHMER VOWEL INHERENT AQ to KHMER VOWEL INHERENT AA */
    {0x17b7, 0x17bd},	/* KHMER VOWEL SIGN I to KHMER VOWEL SIGN UA */
    {0x17c6, 0x17c6},	/* KHMER SIGN NIKAHIT to KHMER SIGN NIKAHIT */
    {0x17c9, 0x17d3},	/* KHMER SIGN MUUSIKATOAN to KHMER SIGN BATHAMASAT */
    {0x17dd, 0x17dd},	/* KHMER SIGN ATTHACAN to KHMER SIGN ATTHACAN */
    {0x180b, 0x180e},	/* MONGOLIAN FREE VARIATION SELECTOR ONE to MONGOLIAN VOWEL SEPARATOR */
    {0x18a9, 0x18a9},	/* MONGOLIAN LETTER ALI GALI DAGALGA to MONGOLIAN LETTER ALI GALI DAGALGA */
    {0x1920, 0x1922},	/* LIMBU VOWEL SIGN A to LIMBU VOWEL SIGN U */
    {0x1927, 0x1928},	/* LIMBU VOWEL SIGN E to LIMBU VOWEL SIGN O */
    {0x1932, 0x1932},	/* LIMBU SMALL LETTER ANUSVARA to LIMBU SMALL LETTER ANUSVARA */
    {0x1939, 0x193b},	/* LIMBU SIGN MUKPHRENG to LIMBU SIGN SA-I */
    {0x1a17, 0x1a18},	/* BUGINESE VOWEL SIGN I to BUGINESE VOWEL SIGN U */
    {0x1a1b, 0x1a1b},	/* BUGINESE VOWEL SIGN AE to BUGINESE VOWEL SIGN AE */
    {0x1a56, 0x1a56},	/* TAI THAM CONSONANT SIGN MEDIAL LA to TAI THAM CONSONANT SIGN MEDIAL LA */
    {0x1a58, 0x1a5e},	/* TAI THAM SIGN MAI KANG LAI to TAI THAM CONSONANT SIGN SA */
    {0x1a60, 0x1a60},	/* TAI THAM SIGN SAKOT to TAI THAM SIGN SAKOT */
    {0x1a62, 0x1a62},	/* TAI THAM VOWEL SIGN MAI SAT to TAI THAM VOWEL SIGN MAI SAT */
    {0x1a65, 0x1a6c},	/* TAI THAM VOWEL SIGN I to TAI THAM VOWEL SIGN OA BELOW */
    {0x1a73, 0x1a7c},	/* TAI THAM VOWEL SIGN OA ABOVE to TAI THAM SIGN KHUEN-LUE KARAN */
    {0x1a7f, 0x1a7f},	/* TAI THAM COMBINING CRYPTOGRAMMIC DOT to TAI THAM COMBINING CRYPTOGRAMMIC DOT */
    {0x1ab0, 0x1abe},	/* COMBINING DOUBLED CIRCUMFLEX ACCENT to COMBINING PARENTHESES OVERLAY */
    {0x1b00, 0x1b03},	/* BALINESE SIGN ULU RICEM to BALINESE SIGN SURANG */
    {0x1b34, 0x1b34},	/* BALINESE SIGN REREKAN to BALINESE SIGN REREKAN */
    {0x1b36, 0x1b3a},	/* BALINESE VOWEL SIGN ULU to BALINESE VOWEL SIGN RA REPA */
    {0x1b3c, 0x1b3c},	/* BALINESE VOWEL SIGN LA LENGA to BALINESE VOWEL SIGN LA LENGA */
    {0x1b42, 0x1b42},	/* BALINESE VOWEL SIGN PEPET to BALINESE VOWEL SIGN PEPET */
    {0x1b6b, 0x1b73},	/* BALINESE MUSICAL SYMBOL COMBINING TEGEH to BALINESE MUSICAL SYMBOL COMBINING GONG */
    {0x1b80, 0x1b81},	/* SUNDANESE SIGN PANYECEK to SUNDANESE SIGN PANGLAYAR */
    {0x1ba2, 0x1ba5},	/* SUNDANESE CONSONANT SIGN PANYAKRA to SUNDANESE VOWEL SIGN PANYUKU */
    {0x1ba8, 0x1ba9},	/* SUNDANESE VOWEL SIGN PAMEPET to SUNDANESE VOWEL SIGN PANEULEUNG */
    {0x1bab, 0x1bad},	/* SUNDANESE SIGN VIRAMA to SUNDANESE CONSONANT SIGN PASANGAN WA */
    {0x1be6, 0x1be6},	/* BATAK SIGN TOMPI to BATAK SIGN TOMPI */
    {0x1be8, 0x1be9},	/* BATAK VOWEL SIGN PAKPAK E to BATAK VOWEL SIGN EE */
    {0x1bed, 0x1bed},	/* BATAK VOWEL SIGN KARO O to BATAK VOWEL SIGN KARO O */
    {0x1bef, 0x1bf1},	/* BATAK VOWEL SIGN U FOR SIMALUNGUN SA to BATAK CONSONANT SIGN H */
    {0x1c2c, 0x1c33},	/* LEPCHA VOWEL SIGN E to LEPCHA CONSONANT SIGN T */
    {0x1c36, 0x1c37},	/* LEPCHA SIGN RAN to LEPCHA SIGN NUKTA */
    {0x1cd0, 0x1cd2},	/* VEDIC TONE KARSHANA to VEDIC TONE PRENKHA */
    {0x1cd4, 0x1ce0},	/* VEDIC SIGN YAJURVEDIC MIDLINE SVARITA to VEDIC TONE RIGVEDIC KASHMIRI INDEPENDENT SVARITA */
    {0x1ce2, 0x1ce8},	/* VEDIC SIGN VISARGA SVARITA to VEDIC SIGN VISARGA ANUDATTA WITH TAIL */
    {0x1ced, 0x1ced},	/* VEDIC SIGN TIRYAK to VEDIC SIGN TIRYAK */
    {0x1cf4, 0x1cf4},	/* VEDIC TONE CANDRA ABOVE to VEDIC TONE CANDRA ABOVE */
    {0x1cf8, 0x1cf9},	/* VEDIC TONE RING ABOVE to VEDIC TONE DOUBLE RING ABOVE */
    {0x1dc0, 0x1df5},	/* COMBINING DOTTED GRAVE ACCENT to COMBINING UP TACK ABOVE */
    {0x1dfc, 0x1dff},	/* COMBINING DOUBLE INVERTED BREVE BELOW to COMBINING RIGHT ARROWHEAD AND DOWN ARROWHEAD BELOW */
    {0x200b, 0x200f},	/* ZERO WIDTH SPACE to RIGHT-TO-LEFT MARK */
    {0x202a, 0x202e},	/* LEFT-TO-RIGHT EMBEDDING to RIGHT-TO-LEFT OVERRIDE */
    {0x2060, 0x2064},	/* WORD JOINER to INVISIBLE PLUS */
    {0x2066, 0x206f},	/* LEFT-TO-RIGHT ISOLATE to NOMINAL DIGIT SHAPES */
    {0x20d0, 0x20f0},	/* COMBINING LEFT HARPOON ABOVE to COMBINING ASTERISK ABOVE */
    {0x2cef, 0x2cf1},	/* COPTIC COMBINING NI ABOVE to COPTIC COMBINING SPIRITUS LENIS */
    {0x2d7f, 0x2d7f},	/* TIFINAGH CONSONANT JOINER to TIFINAGH CONSONANT JOINER */
    {0x2de0, 0x2dff},	/* COMBINING CYRILLIC LETTER BE to COMBINING CYRILLIC LETTER IOTIFIED BIG YUS */
    {0x302a, 0x302d},	/* IDEOGRAPHIC LEVEL TONE MARK to IDEOGRAPHIC ENTERING TONE MARK */
    {0x3099, 0x309a},	/* COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK to COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
    {0xa66f, 0xa672},	/* COMBINING CYRILLIC VZMET to COMBINING CYRILLIC THOUSAND MILLIONS SIGN */
    {0xa674, 0xa67d},	/* COMBINING CYRILLIC LETTER UKRAINIAN IE to COMBINING CYRILLIC PAYEROK */
    {0xa69e, 0xa69f},	/* COMBINING CYRILLIC LETTER EF to COMBINING CYRILLIC LETTER IOTIFIED E */
    {0xa6f0, 0xa6f1},	/* BAMUM COMBINING MARK KOQNDON to BAMUM COMBINING MARK TUKWENTIS */
    {0xa802, 0xa802},	/* SYLOTI NAGRI SIGN DVISVARA to SYLOTI NAGRI SIGN DVISVARA */
    {0xa806, 0xa806},	/* SYLOTI NAGRI SIGN HASANTA to SYLOTI NAGRI SIGN HASANTA */
    {0xa80b, 0xa80b},	/* SYLOTI NAGRI SIGN ANUSVARA to SYLOTI NAGRI SIGN ANUSVARA */
    {0xa825, 0xa826},	/* SYLOTI NAGRI VOWEL SIGN U to SYLOTI NAGRI VOWEL SIGN E */
    {0xa8c4, 0xa8c4},	/* SAURASHTRA SIGN VIRAMA to SAURASHTRA SIGN VIRAMA */
    {0xa8e0, 0xa8f1},	/* COMBINING DEVANAGARI DIGIT ZERO to COMBINING DEVANAGARI SIGN AVAGRAHA */
    {0xa926, 0xa92d},	/* KAYAH LI VOWEL UE to KAYAH LI TONE CALYA PLOPHU */
    {0xa947, 0xa951},	/* REJANG VOWEL SIGN I to REJANG CONSONANT SIGN R */
    {0xa980, 0xa982},	/* JAVANESE SIGN PANYANGGA to JAVANESE SIGN LAYAR */
    {0xa9b3, 0xa9b3},	/* JAVANESE SIGN CECAK TELU to JAVANESE SIGN CECAK TELU */
    {0xa9b6, 0xa9b9},	/* JAVANESE VOWEL SIGN WULU to JAVANESE VOWEL SIGN SUKU MENDUT */
    {0xa9bc, 0xa9bc},	/* JAVANESE VOWEL SIGN PEPET to JAVANESE VOWEL SIGN PEPET */
    {0xa9e5, 0xa9e5},	/* MYANMAR SIGN SHAN SAW to MYANMAR SIGN SHAN SAW */
    {0xaa29, 0xaa2e},	/* CHAM VOWEL SIGN AA to CHAM VOWEL SIGN OE */
    {0xaa31, 0xaa32},	/* CHAM VOWEL SIGN AU to CHAM VOWEL SIGN UE */
    {0xaa35, 0xaa36},	/* CHAM CONSONANT SIGN LA to CHAM CONSONANT SIGN WA */
    {0xaa43, 0xaa43},	/* CHAM CONSONANT SIGN FINAL NG to CHAM CONSONANT SIGN FINAL NG */
    {0xaa4c, 0xaa4c},	/* CHAM CONSONANT SIGN FINAL M to CHAM CONSONANT SIGN FINAL M */
    {0xaa7c, 0xaa7c},	/* MYANMAR SIGN TAI LAING TONE-2 to MYANMAR SIGN TAI LAING TONE-2 */
    {0xaab0, 0xaab0},	/* TAI VIET MAI KANG to TAI VIET MAI KANG */
    {0xaab2, 0xaab4},	/* TAI VIET VOWEL I to TAI VIET VOWEL U */
    {0xaab7, 0xaab8},	/* TAI VIET MAI KHIT to TAI VIET VOWEL IA */
    {0xaabe, 0xaabf},	/* TAI VIET VOWEL AM to TAI VIET TONE MAI EK */
    {0xaac1, 0xaac1},	/* TAI VIET TONE MAI THO to TAI VIET TONE MAI THO */
    {0xaaec, 0xaaed},	/* MEETEI MAYEK VOWEL SIGN UU to MEETEI MAYEK VOWEL SIGN AAI */
    {0xaaf6, 0xaaf6},	/* MEETEI MAYEK VIRAMA to MEETEI MAYEK VIRAMA */
    {0xabe5, 0xabe5},	/* MEETEI MAYEK VOWEL SIGN ANAP to MEETEI MAYEK VOWEL SIGN ANAP */
    {0xabe8, 0xabe8},	/* MEETEI MAYEK VOWEL SIGN UNAP to MEETEI MAYEK VOWEL SIGN UNAP */
    {0xabed, 0xabed},	/* MEETEI MAYEK APUN IYEK to MEETEI MAYEK APUN IYEK */
    {0xfb1e, 0xfb1e},	/* HEBREW POINT JUDEO-SPANISH VARIKA to HEBREW POINT JUDEO-SPANISH VARIKA */
    {0xfe00, 0xfe0f},	/* VARIATION SELECTOR-1 to VARIATION SELECTOR-16 */
    {0xfe20, 0xfe2f},	/* COMBINING LIGATURE LEFT HALF to COMBINING CYRILLIC TITLO RIGHT HALF */
    {0xfeff, 0xfeff},	/* ZERO WIDTH NO-BREAK SPACE to ZERO WIDTH NO-BREAK SPACE */
    {0xfff9, 0xfffb},	/* INTERLINEAR ANNOTATION ANCHOR to INTERLINEAR ANNOTATION TERMINATOR */
    {0x101fd, 0x101fd},	/* PHAISTOS DISC SIGN COMBINING OBLIQUE STROKE to PHAISTOS DISC SIGN COMBINING OBLIQUE STROKE */
    {0x102e0, 0x102e0},	/* COPTIC EPACT THOUSANDS MARK to COPTIC EPACT THOUSANDS MARK */
    {0x10376, 0x1037a},	/* COMBINING OLD PERMIC LETTER AN to COMBINING OLD PERMIC LETTER SII */
    {0x10a01, 0x10a03},	/* KHAROSHTHI VOWEL SIGN I to KHAROSHTHI VOWEL SIGN VOCALIC R */
    {0x10a05, 0x10a06},	/* KHAROSHTHI VOWEL SIGN E to KHAROSHTHI VOWEL SIGN O */
    {0x10a0c, 0x10a0f},	/* KHAROSHTHI VOWEL LENGTH MARK to KHAROSHTHI SIGN VISARGA */
    {0x10a38, 0x10a3a},	/* KHAROSHTHI SIGN BAR ABOVE to KHAROSHTHI SIGN DOT BELOW */
    {0x10a3f, 0x10a3f},	/* KHAROSHTHI VIRAMA to KHAROSHTHI VIRAMA */
    {0x10ae5, 0x10ae6},	/* MANICHAEAN ABBREVIATION MARK ABOVE to MANICHAEAN ABBREVIATION MARK BELOW */
    {0x11001, 0x11001},	/* BRAHMI SIGN ANUSVARA to BRAHMI SIGN ANUSVARA */
    {0x11038, 0x11046},	/* BRAHMI VOWEL SIGN AA to BRAHMI VIRAMA */
    {0x1107f, 0x11081},	/* BRAHMI NUMBER JOINER to KAITHI SIGN ANUSVARA */
    {0x110b3, 0x110b6},	/* KAITHI VOWEL SIGN U to KAITHI VOWEL SIGN AI */
    {0x110b9, 0x110ba},	/* KAITHI SIGN VIRAMA to KAITHI SIGN NUKTA */
    {0x110bd, 0x110bd},	/* KAITHI NUMBER SIGN to KAITHI NUMBER SIGN */
    {0x11100, 0x11102},	/* CHAKMA SIGN CANDRABINDU to CHAKMA SIGN VISARGA */
    {0x11127, 0x1112b},	/* CHAKMA VOWEL SIGN A to CHAKMA VOWEL SIGN UU */
    {0x1112d, 0x11134},	/* CHAKMA VOWEL SIGN AI to CHAKMA MAAYYAA */
    {0x11173, 0x11173},	/* MAHAJANI SIGN NUKTA to MAHAJANI SIGN NUKTA */
    {0x11180, 0x11181},	/* SHARADA SIGN CANDRABINDU to SHARADA SIGN ANUSVARA */
    {0x111b6, 0x111be},	/* SHARADA VOWEL SIGN U to SHARADA VOWEL SIGN O */
    {0x111ca, 0x111cc},	/* SHARADA SIGN NUKTA to SHARADA EXTRA SHORT VOWEL MARK */
    {0x1122f, 0x11231},	/* KHOJKI VOWEL SIGN U to KHOJKI VOWEL SIGN AI */
    {0x11234, 0x11234},	/* KHOJKI SIGN ANUSVARA to KHOJKI SIGN ANUSVARA */
    {0x11236, 0x11237},	/* KHOJKI SIGN NUKTA to KHOJKI SIGN SHADDA */
    {0x112df, 0x112df},	/* KHUDAWADI SIGN ANUSVARA to KHUDAWADI SIGN ANUSVARA */
    {0x112e3, 0x112ea},	/* KHUDAWADI VOWEL SIGN U to KHUDAWADI SIGN VIRAMA */
    {0x11300, 0x11301},	/* GRANTHA SIGN COMBINING ANUSVARA ABOVE to GRANTHA SIGN CANDRABINDU */
    {0x1133c, 0x1133c},	/* GRANTHA SIGN NUKTA to GRANTHA SIGN NUKTA */
    {0x11340, 0x11340},	/* GRANTHA VOWEL SIGN II to GRANTHA VOWEL SIGN II */
    {0x11366, 0x1136c},	/* COMBINING GRANTHA DIGIT ZERO to COMBINING GRANTHA DIGIT SIX */
    {0x11370, 0x11374},	/* COMBINING GRANTHA LETTER A to COMBINING GRANTHA LETTER PA */
    {0x114b3, 0x114b8},	/* TIRHUTA VOWEL SIGN U to TIRHUTA VOWEL SIGN VOCALIC LL */
    {0x114ba, 0x114ba},	/* TIRHUTA VOWEL SIGN SHORT E to TIRHUTA VOWEL SIGN SHORT E */
    {0x114bf, 0x114c0},	/* TIRHUTA SIGN CANDRABINDU to TIRHUTA SIGN ANUSVARA */
    {0x114c2, 0x114c3},	/* TIRHUTA SIGN VIRAMA to TIRHUTA SIGN NUKTA */
    {0x115b2, 0x115b5},	/* SIDDHAM VOWEL SIGN U to SIDDHAM VOWEL SIGN VOCALIC RR */
    {0x115bc, 0x115bd},	/* SIDDHAM SIGN CANDRABINDU to SIDDHAM SIGN ANUSVARA */
    {0x115bf, 0x115c0},	/* SIDDHAM SIGN VIRAMA to SIDDHAM SIGN NUKTA */
    {0x115dc, 0x115dd},	/* SIDDHAM VOWEL SIGN ALTERNATE U to SIDDHAM VOWEL SIGN ALTERNATE UU */
    {0x11633, 0x1163a},	/* MODI VOWEL SIGN U to MODI VOWEL SIGN AI */
    {0x1163d, 0x1163d},	/* MODI SIGN ANUSVARA to MODI SIGN ANUSVARA */
    {0x1163f, 0x11640},	/* MODI SIGN VIRAMA to MODI SIGN ARDHACANDRA */
    {0x116ab, 0x116ab},	/* TAKRI SIGN ANUSVARA to TAKRI SIGN ANUSVARA */
    {0x116ad, 0x116ad},	/* TAKRI VOWEL SIGN AA to TAKRI VOWEL SIGN AA */
    {0x116b0, 0x116b5},	/* TAKRI VOWEL SIGN U to TAKRI VOWEL SIGN AU */
    {0x116b7, 0x116b7},	/* TAKRI SIGN NUKTA to TAKRI SIGN NUKTA */
    {0x1171d, 0x1171f},	/* AHOM CONSONANT SIGN MEDIAL LA to AHOM CONSONANT SIGN MEDIAL LIGATING RA */
    {0x11722, 0x11725},	/* AHOM VOWEL SIGN I to AHOM VOWEL SIGN UU */
    {0x11727, 0x1172b},	/* AHOM VOWEL SIGN AW to AHOM SIGN KILLER */
    {0x16af0, 0x16af4},	/* BASSA VAH COMBINING HIGH TONE to BASSA VAH COMBINING HIGH-LOW TONE */
    {0x16b30, 0x16b36},	/* PAHAWH HMONG MARK CIM TUB to PAHAWH HMONG MARK CIM TAUM */
    {0x16f8f, 0x16f92},	/* MIAO TONE RIGHT to MIAO TONE BELOW */
    {0x1bc9d, 0x1bc9e},	/* DUPLOYAN THICK LETTER SELECTOR to DUPLOYAN DOUBLE MARK */
    {0x1bca0, 0x1bca3},	/* SHORTHAND FORMAT LETTER OVERLAP to SHORTHAND FORMAT UP STEP */
    {0x1d167, 0x1d169},	/* MUSICAL SYMBOL COMBINING TREMOLO-1 to MUSICAL SYMBOL COMBINING TREMOLO-3 */
    {0x1d173, 0x1d182},	/* MUSICAL SYMBOL BEGIN BEAM to MUSICAL SYMBOL COMBINING LOURE */
    {0x1d185, 0x1d18b},	/* MUSICAL SYMBOL COMBINING DOIT to MUSICAL SYMBOL COMBINING TRIPLE TONGUE */
    {0x1d1aa, 0x1d1ad},	/* MUSICAL SYMBOL COMBINING DOWN BOW to MUSICAL SYMBOL COMBINING SNAP PIZZICATO */
    {0x1d242, 0x1d244},	/* COMBINING GREEK MUSICAL TRISEME to COMBINING GREEK MUSICAL PENTASEME */
    {0x1da00, 0x1da36},	/* SIGNWRITING HEAD RIM to SIGNWRITING AIR SUCKING IN */
    {0x1da3b, 0x1da6c},	/* SIGNWRITING MOUTH CLOSED NEUTRAL to SIGNWRITING EXCITEMENT */
    {0x1da75, 0x1da75},	/* SIGNWRITING UPPER BODY TILTING FROM HIP JOINTS to SIGNWRITING UPPER BODY TILTING FROM HIP JOINTS */
    {0x1da84, 0x1da84},	/* SIGNWRITING LOCATION HEAD NECK to SIGNWRITING LOCATION HEAD NECK */
    {0x1da9b, 0x1da9f},	/* SIGNWRITING FILL MODIFIER-2 to SIGNWRITING FILL MODIFIER-6 */
    {0x1daa1, 0x1daaf},	/* SIGNWRITING ROTATION MODIFIER-2 to SIGNWRITING ROTATION MODIFIER-16 */
    {0x1e8d0, 0x1e8d6},	/* MENDE KIKAKUI COMBINING NUMBER TEENS to MENDE KIKAKUI COMBINING NUMBER MILLIONS */
    {0xe0001, 0xe0001},	/* LANGUAGE TAG to LANGUAGE TAG */
    {0xe0020, 0xe007f},	/* TAG SPACE to CANCEL TAG */
    {0xe0100, 0xe01ef},	/* VARIATION SELECTOR-17 to VARIATION SELECTOR-256 */
  };

  /* sorted list of non-overlapping intervals of wide characters */
  /* All 'W' and 'F' characters from version 8.0.0 of
     http://www.unicode.org/Public/UNIDATA/EastAsianWidth.txt
   */

  static const struct interval wide[] = {
    {0x1100, 0x115f},	/* HANGUL CHOSEONG KIYEOK to HANGUL CHOSEONG FILLER */
    {0x2329, 0x232a},	/* LEFT-POINTING ANGLE BRACKET to RIGHT-POINTING ANGLE BRACKET */
    {0x2e80, 0x2e99},	/* CJK RADICAL REPEAT to CJK RADICAL RAP */
    {0x2e9b, 0x2ef3},	/* CJK RADICAL CHOKE to CJK RADICAL C-SIMPLIFIED TURTLE */
    {0x2f00, 0x2fd5},	/* KANGXI RADICAL ONE to KANGXI RADICAL FLUTE */
    {0x2ff0, 0x2ffb},	/* IDEOGRAPHIC DESCRIPTION CHARACTER LEFT TO RIGHT to IDEOGRAPHIC DESCRIPTION CHARACTER OVERLAID */
    {0x3000, 0x303e},	/* IDEOGRAPHIC SPACE to IDEOGRAPHIC VARIATION INDICATOR */
    {0x3041, 0x3096},	/* HIRAGANA LETTER SMALL A to HIRAGANA LETTER SMALL KE */
    {0x3099, 0x30ff},	/* COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK to KATAKANA DIGRAPH KOTO */
    {0x3105, 0x312d},	/* BOPOMOFO LETTER B to BOPOMOFO LETTER IH */
    {0x3131, 0x318e},	/* HANGUL LETTER KIYEOK to HANGUL LETTER ARAEAE */
    {0x3190, 0x31ba},	/* IDEOGRAPHIC ANNOTATION LINKING MARK to BOPOMOFO LETTER ZY */
    {0x31c0, 0x31e3},	/* CJK STROKE T to CJK STROKE Q */
    {0x31f0, 0x321e},	/* KATAKANA LETTER SMALL KU to PARENTHESIZED KOREAN CHARACTER O HU */
    {0x3220, 0x3247},	/* PARENTHESIZED IDEOGRAPH ONE to CIRCLED IDEOGRAPH KOTO */
    {0x3250, 0x32fe},	/* PARTNERSHIP SIGN to CIRCLED KATAKANA WO */
    {0x3300, 0x4dbf},	/* SQUARE APAATO to  */
    {0x4e00, 0xa48c},	/* <CJK Ideograph, First> to YI SYLLABLE YYR */
    {0xa490, 0xa4c6},	/* YI RADICAL QOT to YI RADICAL KE */
    {0xa960, 0xa97c},	/* HANGUL CHOSEONG TIKEUT-MIEUM to HANGUL CHOSEONG SSANGYEORINHIEUH */
    {0xac00, 0xd7a3},	/* <Hangul Syllable, First> to <Hangul Syllable, Last> */
    {0xf900, 0xfaff},	/* CJK COMPATIBILITY IDEOGRAPH-F900 to  */
    {0xfe10, 0xfe19},	/* PRESENTATION FORM FOR VERTICAL COMMA to PRESENTATION FORM FOR VERTICAL HORIZONTAL ELLIPSIS */
    {0xfe30, 0xfe52},	/* PRESENTATION FORM FOR VERTICAL TWO DOT LEADER to SMALL FULL STOP */
    {0xfe54, 0xfe66},	/* SMALL SEMICOLON to SMALL EQUALS SIGN */
    {0xfe68, 0xfe6b},	/* SMALL REVERSE SOLIDUS to SMALL COMMERCIAL AT */
    {0xff01, 0xff60},	/* FULLWIDTH EXCLAMATION MARK to FULLWIDTH RIGHT WHITE PARENTHESIS */
    {0xffe0, 0xffe6},	/* FULLWIDTH CENT SIGN to FULLWIDTH WON SIGN */
    {0x1b000, 0x1b001},	/* KATAKANA LETTER ARCHAIC E to HIRAGANA LETTER ARCHAIC YE */
    {0x1f200, 0x1f202},	/* SQUARE HIRAGANA HOKA to SQUARED KATAKANA SA */
    {0x1f210, 0x1f23a},	/* SQUARED CJK UNIFIED IDEOGRAPH-624B to SQUARED CJK UNIFIED IDEOGRAPH-55B6 */
    {0x1f240, 0x1f248},	/* TORTOISE SHELL BRACKETED CJK UNIFIED IDEOGRAPH-672C to TORTOISE SHELL BRACKETED CJK UNIFIED IDEOGRAPH-6557 */
    {0x1f250, 0x1f251},	/* CIRCLED IDEOGRAPH ADVANTAGE to CIRCLED IDEOGRAPH ACCEPT */
    {0x20000, 0x2fffd},
    {0x30000, 0x3fffd},
  };

  /* Fast test for 8-bit control characters and many ISO8859 characters. */
  /* NOTE: this overrides the 'Cf' definition of the U+00AD character */
  if (ucs < 0x0300) {
    if (ucs == 0)
      return 0;
    if (ucs < 32 || (ucs >= 0x7f && ucs < 0xa0))
      return -1;
    return 1;
  }

  /* binary search in table of non-spacing characters */
  if (bisearch(ucs, combining,
	       sizeof(combining) / sizeof(struct interval) - 1))
    return 0;

  /* The first wide character is U+1100, everything below it is 'normal'. */
  if (ucs < 0x1100)
    return 1;

  /* Hangul Jamo medial vowels and final consonants (U+1160-U+11FF)
   * are zero length despite not being Mn, Me or Cf */
  if (ucs >= 0x1160 && ucs <= 0x11FF)
    return 0;

  /* if we arrive here, ucs is not a combining or C0/C1 control character */
  return 1 + (bisearch(ucs, wide, sizeof(wide) / sizeof(struct interval) - 1));
}


int mk_wcswidth(const unsigned int *pwcs, size_t n)
{
  int w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = mk_wcwidth(*pwcs)) < 0)
      return -1;
    else
      width += w;

  return width;
}


/*
 * Rather than updating these functions to Unicode 8 I have changed
 * them so they match the only use-case we have. This width table is
 * for data that has been converted from on of the old DBCS codepages
 * and so every chraracter that can be printed in the set that is not in
 * the ASCII set or an alternate for the ASCII set will be double width.
 * It is not otherwise recommended for general use.
 */
int mk_wcwidth_cjk(unsigned int ucs)
{
  int w = mk_wcwidth(ucs);
  if (w == 1 && ucs >= 0x00A1 && ucs < 0xFF61 && ucs != 0x20A9)
    return 2;
  else
    return w;
}


int mk_wcswidth_cjk(const unsigned int *pwcs, size_t n)
{
  int w, width = 0;

  for (;*pwcs && n-- > 0; pwcs++)
    if ((w = mk_wcwidth_cjk(*pwcs)) < 0)
      return -1;
    else
      width += w;

  return width;
}

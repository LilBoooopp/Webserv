#ifndef COLORS_HPP
#define COLORS_HPP

#define TS "\x1b[0m"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define rgb(r, g, b) "\x1b[38;2;" STR(r) ";" STR(g) ";" STR(b) "m"
#define rgba(r, g, b, a) "\x1b[38;2;" STR(r) ";" STR(g) ";" STR(b) "m"

#define YELLOW rgb(195, 186, 66)
#define RED rgb(190, 57, 57)
#define GREEN rgb(83, 223, 83)
#define BLUE rgb(72, 72, 198)
#define PURPLE rgb(141, 40, 199)
#define PINK rgb(187, 52, 164)
#define BROWN rgb(74, 51, 20)
#define GREY rgb(137, 137, 137)
#define LGREY rgb(182, 167, 167)
#define URLCLR rgb(171, 173, 194)
#define TIMECLR rgb(186, 171, 194)
#define VALUECLR rgb(170, 187, 214)
#define SERVCLR rgb(129, 166, 192)
#define SERV_CLR "\033[38;2;129;166;192mServer\033[0m"

#define HTTPCLR rgb(129, 192, 129)

#define BLD "\x1b[1m"
#define TSBLD "\x1b[0m"

#endif

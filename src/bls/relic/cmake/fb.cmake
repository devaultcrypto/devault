message(STATUS "Binary field arithmetic configuration (FB module):")
if (SHOW_MESSAGES) 

message("   ** Options for the binary elliptic curve module (default = 283,0,on,on,on):")

message("      FB_POLYN=n        The irreducible polynomial size in bits.")
message("      FB_KARAT=n        The number of Karatsuba levels.")
message("      FB_TRINO=[off|on] Prefer trinomials.")
message("      FB_SQRTF=[off|on] Prefer square-root friendly polynomials.")
message("      FB_PRECO=[off|on] Precompute multiplication table for sqrt(z).")
message("      FB_WIDTH=w        Width w in [2,6] of window processing for exponentiation methods.")

message("   ** Available binary field arithmetic methods (default = LODAH;LUTBL;QUICK;BASIC;QUICK;QUICK;EXGCD;SLIDE;QUICK):")

message("      Field multiplication:")
message("      FB_METHD=BASIC    Right-to-left shift-and-add multiplication.")
message("      FB_METHD=INTEG    Integrated modular multiplication.")
message("      FB_METHD=RCOMB    Right-to-left comb multiplication.")
message("      FB_METHD=LCOMB    Left-to-right comb multiplication.")
message("      FB_METHD=LODAH    López-Dahab comb multiplication with window of width 4.")

message("      Field squaring:")
message("      FB_METHD=BASIC    Bit manipulation squaring.")
message("      FB_METHD=INTEG    Integrated modular squaring.")
message("      FB_METHD=LUTBL    Table-based squaring.")

message("      Modular reduction:")
message("      FB_METHD=BASIC    Shift-and-add modular reduction.")
message("      FB_METHD=QUICK    Fast reduction modulo a trinomial or pentanomial.")

message("      Field square root:")
message("      FB_METHD=BASIC    Square root by repeated squaring.")
message("      FB_METHD=QUICK    Fast square root extraction.")

message("      Trace computation:")
message("      FB_METHD=BASIC    Trace computation by repeated squaring.")
message("      FB_METHD=QUICK    Fast trace computation.")

message("      Quadratic equation solver:")
message("      FB_METHD=BASIC    Solve a quadratic equation by half-trace computation.")
message("      FB_METHD=QUICK    Fast solving with precomputed half-traces.")

message("      Field inversion:")
message("      FB_METHD=BASIC    Inversion by Fermat's Little Theorem.")
message("      FB_METHD=BINAR    Binary Inversion algorithm.")
message("      FB_METHD=ALMOS    Inversion by the Amost inverse algorithm.")
message("      FB_METHD=EXGCD    Inversion by the Extended Euclidean algorithm.")
message("      FB_METHD=ITOHT    Inversion by Itoh-Tsuji.")
message("      FB_METHD=BRUCH    Hardware-friendly inversion by Brunner et al.")
message("      FB_METHD=LOWER    Pass inversion to the lower level.")

message("      Field exponentiation:")
message("      FB_METHD=BASIC    Binary exponentiation.")
message("      FB_METHD=SLIDE    Sliding window exponentiation.")
message("      FB_METHD=MONTY    Constant-time Montgomery powering ladder.")

message("      Iterated squaring/square-root:")
message("      FB_METHD=BASIC    Iterated squaring/square-root by consecutive squaring/square-root.")
message("      FB_METHD=QUICK    Iterated squaring/square-root by table-based method.")
endif()
# Choose the polynomial size.
set(FB_POLYN 283)

# Fix the number of Karatsuba instances
set(FB_KARAT 0)

set(FB_WIDTH 4)

option(FB_TRINO "Prefer trinomials." on)
option(FB_SQRTF "Prefer square-root friendly polynomials." off)
option(FB_PRECO "Precompute multiplication table for sqrt(z)." on)

# Choose the arithmetic methods.
if (NOT FB_METHD)
	set(FB_METHD "LODAH;LUTBL;QUICK;QUICK;QUICK;QUICK;EXGCD;SLIDE;QUICK")
endif(NOT FB_METHD)
list(LENGTH FB_METHD FB_LEN)
if (FB_LEN LESS 9)
	message(FATAL_ERROR "Incomplete FB_METHD specification: ${FB_METHD}")
endif(FB_LEN LESS 9)

list(GET FB_METHD 0 FB_MUL)
list(GET FB_METHD 1 FB_SQR)
list(GET FB_METHD 2 FB_RDC)
list(GET FB_METHD 3 FB_SRT)
list(GET FB_METHD 4 FB_TRC)
list(GET FB_METHD 5 FB_SLV)
list(GET FB_METHD 6 FB_INV)
list(GET FB_METHD 7 FB_EXP)
list(GET FB_METHD 8 FB_ITR)

set(FB_METHD ${FB_METHD} CACHE STRING "Method for binary field arithmetic.")

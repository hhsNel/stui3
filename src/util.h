#ifndef UTIL_H
#define UTIL_H

#define STUI3_ENOENT (1)
#define STUI3_EUPSTM (2)
#define STUI3_EIACTN (3)
#define STUI3_EIDATA (4)

#define _STR(X) #X
#define STR(X) _STR(X)

#define _EVAL(X) X
#define EVAL(X) _EVAL(X)

#define _CONCAT(A,B) A##B
#define CONCAT(A,B) _CONCAT(A,B)
#define CONCAT3(A,B,C) CONCAT(A,CONCAT(B,C))
#define CONCAT4(A,B,C,D) CONCAT(A,CONCAT(B,CONCAT(C,D)))
#define CONCAT5(A,B,C,D,E) CONCAT(A,CONCAT(B,CONCAT(C,CONCAT(D,E))))

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

#define __FOREACH_MACRO_0(__MACRO, ...)
#define __FOREACH_MACRO_1(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_0(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_2(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_1(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_3(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_2(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_4(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_3(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_5(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_4(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_6(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_5(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_7(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_6(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_8(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_7(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_9(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_8(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_10(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_9(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_11(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_10(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_12(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_11(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_13(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_12(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_14(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_13(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_15(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_14(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_16(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_15(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_17(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_16(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_18(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_17(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_19(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_18(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_20(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_19(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_21(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_20(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_22(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_21(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_23(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_22(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_24(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_23(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_25(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_24(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_26(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_25(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_27(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_26(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_28(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_27(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_29(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_28(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_30(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_29(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_31(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_30(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_32(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_31(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_33(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_32(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_34(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_33(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_35(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_34(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_36(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_35(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_37(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_36(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_38(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_37(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_39(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_38(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_40(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_39(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_41(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_40(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_42(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_41(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_43(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_42(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_44(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_43(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_45(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_44(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_46(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_45(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_47(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_46(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_48(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_47(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_49(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_48(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_50(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_49(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_51(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_50(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_52(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_51(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_53(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_52(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_54(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_53(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_55(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_54(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_56(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_55(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_57(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_56(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_58(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_57(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_59(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_58(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_60(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_59(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_61(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_60(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_62(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_61(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_63(__MACRO, EL, ...) __MACRO(EL) __FOREACH_MACRO_62(__MACRO, __VA_ARGS__)
#define __FOREACH_MACRO_N(__MACRO, ...) \
    __FOREACH_MACRO_N_(__MACRO, __VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define __FOREACH_MACRO_N_(__MACRO, __0, __1, __2, __3, __4, __5, __6, __7, __8, __9, __10, __11, __12, __13, __14, __15, __16, __17, __18, __19, __20, __21, __22, __23, __24, __25, __26, __27, __28, __29, __30, __31, __32, __33, __34, __35, __36, __37, __38, __39, __40, __41, __42, __43, __44, __45, __46, __47, __48, __49, __50, __51, __52, __53, __54, __55, __56, __57, __58, __59, __60, __61, __62, __63, N, ...) CONCAT(__FOREACH_MACRO_,N)(__MACRO, __0, __1, __2, __3, __4, __5, __6, __7, __8, __9, __10, __11, __12, __13, __14, __15, __16, __17, __18, __19, __20, __21, __22, __23, __24, __25, __26, __27, __28, __29, __30, __31, __32, __33, __34, __35, __36, __37, __38, __39, __40, __41, __42, __43, __44, __45, __46, __47, __48, __49, __50, __51, __52, __53, __54, __55, __56, __57, __58, __59, __60, __61, __62, __63)
#define FOREACH_MACRO(__MACRO, ...) \
    __FOREACH_MACRO_N(__MACRO, __VA_ARGS__)

#define HAS_COMMA(...) __HAS_COMMA(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0)
#define __HAS_COMMA(__1, __2, __3, __4, __5, __6, __7, __8, __9, __10, __11, __12, __13, __14, __15, __16, __17, __18, __19, __20, __21, __22, __23, __24, __25, __26, __27, __28, __29, __30, __31, __32, __33, __34, __35, __36, __37, __38, __39, __40, __41, __42, __43, __44, __45, __46, __47, __48, __49, __50, __51, __52, __53, __54, __55, __56, __57, __58, __59, __60, __61, __62, __63, __64, N, ...) N
#define __TRIGGER_PARENS_(...) ,
#define __EMPTY_CASE_0001 ,
#define __IS_EMPTY(__0,__1,__2,__3) HAS_COMMA(CONCAT5(__EMPTY_CASE_,__0,__1,__2,__3))
#define IS_EMPTY(...) __IS_EMPTY(HAS_COMMA(__VA_ARGS__),HAS_COMMA(__TRIGGER_PARENS_ __VA_ARGS__),HAS_COMMA(__VA_ARGS__()),HAS_COMMA(__TRIGGER_PARENS_ __VA_ARGS__()))

#endif


add_executable(Lesson1 Lesson1.cpp tgaimage.cpp tgaimage.h model.h model.cpp)
add_executable(Lesson2 Lesson2.cpp tgaimage.cpp tgaimage.h model.h model.cpp geometry.h)
add_executable(Lesson3 Lesson3.cpp tgaimage.cpp tgaimage.h model_lesson3.h model_lesson3.cpp geometry.h)
add_executable(Lesson5 Lesson5.cpp tgaimage.cpp tgaimage.h model.h model.cpp geometry.h geometry.cpp)

set(TARGET_HDRS model.h renderer.h tgaimage.h vector.h geometry.h)
set(TARGET_SRCS tgaimage.cpp main.cpp renderer.cpp model.cpp)
# add_executable(tinyrenderer ${TARGET_HDRS} ${TARGET_SRCS})

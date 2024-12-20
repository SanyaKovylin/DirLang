Include = -I Include
COMP_FLAGS = -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Weffc++ -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wctor-dtor-privacy -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers -Wnon-virtual-dtor -Woverloaded-virtual -Wpointer-arith -Wsign-promo -Wstack-usage=8192 -Wstrict-aliasing -Wstrict-null-sentinel -Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE
COMP_FLAGS += $(Include)
CC = g++
MK_FLAGS = -p
RM_FLAGS = -rf

Sources = $(wildcard $(Source)/*.cpp)
Objects = $(patsubst $(Source)/%.cpp,$(Build)/%.o,$(Sources))

Cpp = $(wildcard *.cpp)
H = $(wildcard *.h)

Build = Build
Source = Source
Exe = Compiler.exe

all: $(Build)/$(Exe)


$(Build)/%.o : $(Source)/%.cpp | $(Build)
	$(CC) $(COMP_FLAGS) $< -c -o $@

$(Build)/$(Exe): $(Objects) | $(Build)
	$(CC) $(COMP_FLAGS) -o $(Build)/$(Exe) $(Objects)

$(Build):
	mkdir $(Build)



cleanup:
ifdef Cpp
	mkdir  $(Source)
	move $(Cpp) ./$(Source)
endif
ifdef H
	mkdir Include
	move $(H) Include
endif
	remove -rf $(wildcard *.o)

run:
	$(Build)/$(Exe)

clean:
	rd /s /q $(Build)

test:
	cls





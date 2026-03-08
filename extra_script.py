import os
Import("env")

# Включаем пути тулчейна в compile_commands.json
env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=True)

# Указываем, что файл должен быть в корне проекта
env.Replace(COMPILATIONDB_PATH="compile_commands.json")
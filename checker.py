import os
import filecmp
import sys

def compare_folders(folder1, folder2):
    folder1_files = set(os.listdir(folder1))
    folder2_files = set(os.listdir(folder2))
    
    common_files = folder1_files.intersection(folder2_files)
    
    for file_name in common_files:
        path1 = os.path.join(folder1, file_name)
        path2 = os.path.join(folder2, file_name)
        
        # Если это файл, то сравниваем его содержимое
        if os.path.isfile(path1) and os.path.isfile(path2):
            if filecmp.cmp(path1, path2, shallow=False):
                print(f"Файлы {file_name} одинаковы.")
            else:
                print(f"Файлы {file_name} различаются.")

        # Если это папка, то рекурсивно сравниваем папки
        elif os.path.isdir(path1) and os.path.isdir(path2):
            compare_folders(path1, path2)
        else:
            print(f"Объекты {file_name} различаются (один файл, другой папка).")


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <folder1> <folder2>")
        sys.exit(1)

    folder1 = sys.argv[1]
    folder2 = sys.argv[2]

    if not os.path.isdir(folder1):
        print(f"Ошибка: {folder1} не является директорией.")
        sys.exit(1)
    
    if not os.path.isdir(folder2):
        print(f"Ошибка: {folder2} не является директорией.")
        sys.exit(1)

    compare_folders(folder1, folder2)

if __name__ == '__main__':
    main()
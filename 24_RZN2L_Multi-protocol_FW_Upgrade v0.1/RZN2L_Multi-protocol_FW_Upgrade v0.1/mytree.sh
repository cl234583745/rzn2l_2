#!/bin/bash

# ============================================
# Git Bash 通用树形目录显示工具
# 功能：显示文件夹名，但隐藏其内部内容
# ============================================

# ---------- 默认排除规则 ----------
DEFAULT_EXCLUDE=(
	"*.mk"
	"*.init"
	"*.json"
	"*.in"
    "*.o"
	"*.o.in"
    "*.d"
    "*.obj"
    "*.exe"
    "*.elf"
    "*.hex"
    "*.map"
    "*.lst"
    "*.crf"
    "*.axf"
    "*.sct"
    "*.dep"
    "*.bak"
    "*~"
    "*.user"
    "*.db"
    "*.uvguix.*"
    "*.uvoptx"
    "*.uvprojx"
	"*.jlink"
	"*.log"
	"*.elf.jlink"
	"*.elf.launch"
    "Debug/"
    "Release/"
    "build/"
    ".git/"
    ".svn/"
    ".vs/"
    ".vscode/"
    "*.gitignore"
    "*.gitattributes"
)

# ---------- 新增：需要隐藏内容的目录（只显示目录名，不展开）----------
HIDE_CONTENT_DIRS=(
    "0200-rzn2l-ethercat/"
    "rzn/"
    "rzn_gen/"
    "rzn_cfg/"
    "SSCconfig/"
    "Patch/"
    "ESI/"
    "project/"
    "r_fw_up_rz/"
)

# ---------- 配置文件 ----------
GLOBAL_IGNORE="$HOME/.mytreeignore"
PROJECT_IGNORE=".mytreeignore"

# ---------- 加载排除规则 ----------
EXCLUDE_PATTERNS=("${DEFAULT_EXCLUDE[@]}")

# 加载全局配置
if [ -f "$GLOBAL_IGNORE" ]; then
    while IFS= read -r line || [ -n "$line" ]; do
        [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
        EXCLUDE_PATTERNS+=("$line")
    done < "$GLOBAL_IGNORE"
fi

# 加载项目级配置
if [ -f "$PROJECT_IGNORE" ]; then
    while IFS= read -r line || [ -n "$line" ]; do
        [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue
        EXCLUDE_PATTERNS+=("$line")
    done < "$PROJECT_IGNORE"
fi

# 从项目配置加载需要隐藏内容的目录
if [ -f "$PROJECT_IGNORE" ]; then
    while IFS= read -r line || [ -n "$line" ]; do
        if [[ "$line" =~ ^HIDE_CONTENT: ]]; then
            dir="${line#HIDE_CONTENT:}"
            dir="${dir%"${dir##*[![:space:]]}"}"  # 去除前后空格
            HIDE_CONTENT_DIRS+=("$dir")
        fi
    done < "$PROJECT_IGNORE"
fi

# ---------- 判断是否应该隐藏内容（只显示目录名，不展开）----------
should_hide_content() {
    local dirname="$1"
    
    for pattern in "${HIDE_CONTENT_DIRS[@]}"; do
        # 移除末尾的 /
        pattern="${pattern%/}"
        
        # 匹配目录名
        if [[ "$dirname" == "$pattern" || "$dirname" == */"$pattern" || "$dirname" == "$pattern"/* ]]; then
            return 0
        fi
    done
    return 1
}

# ---------- 判断是否排除（完全隐藏）----------
should_exclude() {
    local path="$1"
    local name="$(basename "$path")"
    
    for pattern in "${EXCLUDE_PATTERNS[@]}"; do
        if [[ "$pattern" == */ ]]; then
            if [[ -d "$path" && "$name" == "${pattern%/}" ]]; then
                return 0
            fi
        elif [[ "$name" == $pattern ]]; then
            return 0
        elif [[ "$path" == *"$pattern"* ]]; then
            return 0
        fi
    done
    return 1
}

# ---------- 颜色定义 ----------
if [ -t 1 ]; then
    COLOR_DIR='\033[1;34m'
    COLOR_HIDE='\033[1;33m'  # 黄色，表示被隐藏内容的目录
    COLOR_FILE='\033[0;32m'
    COLOR_RESET='\033[0m'
else
    COLOR_DIR=''
    COLOR_HIDE=''
    COLOR_FILE=''
    COLOR_RESET=''
fi

# ---------- 核心打印函数（修复版）----------
print_tree() {
    local dir="$1"
    local prefix="$2"
    local full_path="$3"
    local depth="$4"
    
    [ -z "$depth" ] && depth=0
    
    if [ -z "$full_path" ]; then
        full_path="$dir"
    fi
    
    if [ ! -d "$dir" ]; then
        return
    fi
    
    # 检查当前目录是否应该隐藏内容
    local current_dirname="$(basename "$dir")"
    local hide_this_dir=0
    
    if should_hide_content "$current_dirname" || should_hide_content "$full_path"; then
        hide_this_dir=1
    fi
    
    # 如果需要隐藏内容，直接返回，不打印子内容
    if [ $hide_this_dir -eq 1 ] && [ $depth -gt 0 ]; then
        return
    fi
    
    local entries=()
    
    # 获取所有条目
    while IFS= read -r entry; do
        [ -n "$entry" ] && entries+=("$entry")
    done < <(ls -1 "$dir" 2>/dev/null | sort -f)
    
    # 分离目录和文件
    local dirs=()
    local files=()
    
    for entry in "${entries[@]}"; do
        local entry_path="$dir/$entry"
        local full_entry_path="$full_path/$entry"
        
        # 检查是否应该完全排除
        if should_exclude "$entry_path" || should_exclude "$full_entry_path"; then
            continue
        fi
        
        if [ -d "$entry_path" ]; then
            dirs+=("$entry")
        else
            files+=("$entry")
        fi
    done
    
    # 合并目录和文件
    entries=("${dirs[@]}" "${files[@]}")
    local total=${#entries[@]}
    local count=0
    
    for entry in "${entries[@]}"; do
        ((count++))
        local is_last=0
        [ $count -eq $total ] && is_last=1
        
        local entry_path="$dir/$entry"
        local full_entry_path="$full_path/$entry"
        
        if [ -d "$entry_path" ]; then
            # 检查这个子目录是否需要隐藏内容
            local hide_subdir=0
            if should_hide_content "$entry" || should_hide_content "$full_entry_path"; then
                hide_subdir=1
            fi
            
            # 打印目录
            if [ $hide_subdir -eq 1 ]; then
                # 目录内容被隐藏，用黄色显示并添加 [HIDDEN] 标记
                if [ $is_last -eq 1 ]; then
                    echo -e "${prefix}└── ${COLOR_HIDE}$entry${COLOR_RESET}/ [HIDDEN]"
                else
                    echo -e "${prefix}├── ${COLOR_HIDE}$entry${COLOR_RESET}/ [HIDDEN]"
                fi
                # 不递归进入隐藏内容的目录
            else
                if [ $is_last -eq 1 ]; then
                    echo -e "${prefix}└── ${COLOR_DIR}$entry${COLOR_RESET}/"
                    print_tree "$entry_path" "${prefix}    " "$full_entry_path" $((depth + 1))
                else
                    echo -e "${prefix}├── ${COLOR_DIR}$entry${COLOR_RESET}/"
                    print_tree "$entry_path" "${prefix}│   " "$full_entry_path" $((depth + 1))
                fi
            fi
        else
            # 打印文件
            if [ $is_last -eq 1 ]; then
                echo -e "${prefix}└── ${COLOR_FILE}$entry${COLOR_RESET}"
            else
                echo -e "${prefix}├── ${COLOR_FILE}$entry${COLOR_RESET}"
            fi
        fi
    done
}

# ---------- 主函数 ----------
main() {
    local start_dir="${1:-.}"
    
    # 转换路径
    start_dir="${start_dir//\\//}"
    
    if [ ! -d "$start_dir" ]; then
        echo "错误：目录不存在 - $start_dir"
        exit 1
    fi
    
    # 获取绝对路径
    start_dir="$(cd "$start_dir" && pwd)"
    local base_name="$(basename "$start_dir")"
    
    echo -e "${COLOR_DIR}$base_name${COLOR_RESET}/"
    
    local entries=()
    
    # 读取根目录
    while IFS= read -r entry; do
        [ -n "$entry" ] && entries+=("$entry")
    done < <(ls -1 "$start_dir" 2>/dev/null | sort -f)
    
    # 分离根目录的目录和文件
    local dirs=()
    local files=()
    
    for entry in "${entries[@]}"; do
        local entry_path="$start_dir/$entry"
        
        if should_exclude "$entry_path" || should_exclude "/$entry"; then
            continue
        fi
        
        if [ -d "$entry_path" ]; then
            dirs+=("$entry")
        else
            files+=("$entry")
        fi
    done
    
    # 合并并打印根目录内容
    entries=("${dirs[@]}" "${files[@]}")
    local total=${#entries[@]}
    local count=0
    
    for entry in "${entries[@]}"; do
        ((count++))
        local is_last=0
        [ $count -eq $total ] && is_last=1
        
        local entry_path="$start_dir/$entry"
        
        if [ -d "$entry_path" ]; then
            # 检查是否需要隐藏内容
            local hide_this=0
            if should_hide_content "$entry" || should_hide_content "/$entry"; then
                hide_this=1
            fi
            
            if [ $hide_this -eq 1 ]; then
                if [ $is_last -eq 1 ]; then
                    echo -e "└── ${COLOR_HIDE}$entry${COLOR_RESET}/ [HIDDEN]"
                else
                    echo -e "├── ${COLOR_HIDE}$entry${COLOR_RESET}/ [HIDDEN]"
                fi
            else
                if [ $is_last -eq 1 ]; then
                    echo -e "└── ${COLOR_DIR}$entry${COLOR_RESET}/"
                    print_tree "$entry_path" "    " "/$entry" 1
                else
                    echo -e "├── ${COLOR_DIR}$entry${COLOR_RESET}/"
                    print_tree "$entry_path" "│   " "/$entry" 1
                fi
            fi
        else
            if [ $is_last -eq 1 ]; then
                echo -e "└── ${COLOR_FILE}$entry${COLOR_RESET}"
            else
                echo -e "├── ${COLOR_FILE}$entry${COLOR_RESET}"
            fi
        fi
    done
}

# 运行
main "$@"












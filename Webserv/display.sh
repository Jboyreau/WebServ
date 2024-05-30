find . -type f -not -name "*.swp" | while read -r file; do
    echo "File: $file"
    cat "$file"
    echo ""
done

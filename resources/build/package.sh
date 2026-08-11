#!/bin/bash

cd "$(dirname "$0")"
cd ..

package_dir=total_domination-linux
do_cleanup=false
delete_source=false
zip=false
windows=false

OPTIONS=$(getopt -o c,n: --long cleanup,name:,delete-source,zip,windows -- "$@")
eval set -- "$OPTIONS"
while true; do
    case "$1" in
        --cleanup)
            do_cleanup=true
            shift
            ;;
        -n|--name)
            package_dir="$2"
            shift 2
            ;;
        --delete-source)
            delete_source=true
            shift
            ;;
        --zip)
            zip=true
            shift
            ;;
        --windows)
            windows=true
            shift
            ;;
        --) 
            shift
            break
            ;;
        *) 
            echo "Usage: $0 [-n | --name package_name] [--cleanup] [--delete-source]"
            exit 1
            ;;
    esac
done

if [ -d "$package_dir" ]; then 
  rm -r "$package_dir"
fi

mkdir "$package_dir"

if [ "$windows" = false ]; then
    cp bin/total_domination "$package_dir/"
    cp resources/build/run_packaged.sh "$package_dir/run.sh"
    cp -r bin/lib "$package_dir/"
else
    cp bin/total_domination.exe "$package_dir/"
    cp resources/build/run_packaged.bat "$package_dir/run.bat"
    cp -r bin/*.dll "$package_dir/"
fi

cp -r resources "$package_dir/"
rm -rf "$package_dir/resources/build"
cp -r data "$package_dir/"

cp LICENSE "$package_dir/"
cp resources/build/RELEASE_README.md "$package_dir/README.md"

if [ "$zip" = false ]; then
    output_name="$package_dir.tar.gz"
else
    output_name="$package_dir.zip"
fi

if [ -f "$output_name" ]; then 
    rm "$output_name"
fi

if [ "$zip" = false ]; then
    tar -czvf "$output_name" "$package_dir" > /dev/null 2>&1
else
    zip -q -r "$output_name" "$package_dir"
fi

if [ "$do_cleanup" = true ]; then
    echo "Cleaning up.."
    rm -rf "$package_dir"
fi

if [ "$delete_source" = true ]; then
    echo "Deleting compiled source.."
    rm -rf bin/*
fi
mkdir Bulid
copy D:\UnrealEngine\Engine\Source\Programs\Editor_ServerProject\Public\Event\Protobuf\*.proto
protoc.exe ./*.proto --cpp_out=./Bulid
del /F /S /Q *.proto
copy Bulid\*.* D:\UnrealEngine\Engine\Source\Programs\Editor_ServerProject\Public\Event\Protobuf\*.*
rd /s /q Bulid
cd D:\UnrealEngine
GenerateProjectFiles.bat
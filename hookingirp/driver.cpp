
#include <ntifs.h> 

struct DEVICE_EXTENSION
{
    PDEVICE_OBJECT LowerDeviceObject;
};

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, * POBJECT_DIRECTORY_INFORMATION;
extern "C"
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
    _In_ HANDLE DirectoryHandle,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_ BOOLEAN RestartScan,
    _Inout_ PULONG Context,
    _Out_opt_ PULONG ReturnLength
);

void unload(PDRIVER_OBJECT driver);
PDRIVER_OBJECT getdriverpointeur(PUNICODE_STRING objectname);


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry) {
    UNREFERENCED_PARAMETER(registry);
    driver->DriverUnload = unload;
    KdPrint(("succes load \n"));
    UNICODE_STRING name, str;
    RtlInitUnicodeString(&name, L"\\Device\\Null");
    //branche racine du device
    RtlInitUnicodeString(&str, L"\\Device");
    //initialisation structure
    OBJECT_ATTRIBUTES at = { 0 };
    at.Length = sizeof(OBJECT_ATTRIBUTES);
    InitializeObjectAttributes(&at, &str, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    HANDLE hd;
    //ouverture du handle
    NTSTATUS statusopen = ZwOpenDirectoryObject(&hd, DIRECTORY_QUERY, &at);
    if (!NT_SUCCESS(statusopen)) {
        KdPrint(("erreur"));
        return statusopen;
    }
    KdPrint(("adresse handle : %p", hd));

    ULONG context = 0;
    ULONG size = PAGE_SIZE;
    ULONG returnedSize = 0;
    BOOLEAN restart = TRUE;
    KdPrint(("succes "));
    //allocation memoie
    PVOID bufferdirectory = ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'devc');
    if (!bufferdirectory)
    {
        ZwClose(hd);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //utilisation du buffer
    while (TRUE)
    {

     NTSTATUS statusquery2 = ZwQueryDirectoryObject(hd, bufferdirectory, size, TRUE, FALSE, &context, &returnedSize);
    if (statusquery2 == STATUS_NO_MORE_ENTRIES){
        break;
       
    }
    restart = FALSE;

    if (!NT_SUCCESS(statusquery2))
    {
        KdPrint(("erreur"));
        return statusquery2;
    }
    OBJECT_DIRECTORY_INFORMATION * buffercaste = (OBJECT_DIRECTORY_INFORMATION*)bufferdirectory;
   

    
        KdPrint(("nom : %wZ et le  type : %wZ",&buffercaste->Name,&buffercaste->TypeName));

       
    }


    KdPrint(("succes"));
    ExFreePool(bufferdirectory);
    bufferdirectory = NULL;


    ZwClose(hd);
   

    PDRIVER_OBJECT drivercible = getdriverpointeur(&name);

    if (drivercible) {
        KdPrint(("DriverObject = %p\n", drivercible));
    }


    return STATUS_SUCCESS;
}

PDRIVER_OBJECT getdriverpointeur(PUNICODE_STRING objectname ) {
    PFILE_OBJECT fobj;
    PDEVICE_OBJECT deviceobj;
    NTSTATUS status1 = IoGetDeviceObjectPointer(objectname, FILE_ALL_ACCESS, &fobj, &deviceobj);
    if (!NT_SUCCESS(status1)) {
        return nullptr;

    }


    return deviceobj->DriverObject;
}


void unload(PDRIVER_OBJECT driver) {
    UNREFERENCED_PARAMETER(driver);
    KdPrint(("succes unload \n"));
}


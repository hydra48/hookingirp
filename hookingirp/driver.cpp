
#include <ntifs.h> 

PDRIVER_OBJECT drivercibleglobal = nullptr;
PDEVICE_OBJECT dev;


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
NTSTATUS Hookedioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS routine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID context);
NTSTATUS PassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS Hookedread(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS Hookedwrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry) {
    UNREFERENCED_PARAMETER(registry);
    driver->DriverUnload = unload;
    KdPrint(("succes load \n"));
    UNICODE_STRING name, str;
    RtlInitUnicodeString(&name, L"\\Device\\MountPointManager");
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
        if (statusquery2 == STATUS_NO_MORE_ENTRIES) {
            break;

        }
        restart = FALSE;

        if (!NT_SUCCESS(statusquery2))
        {
            KdPrint(("erreur"));
            return statusquery2;
        }
        OBJECT_DIRECTORY_INFORMATION* buffercaste = (OBJECT_DIRECTORY_INFORMATION*)bufferdirectory;



        KdPrint(("nom : %wZ et le  type : %wZ", &buffercaste->Name, &buffercaste->TypeName));


    }


    KdPrint(("succes"));
    ExFreePool(bufferdirectory);
    bufferdirectory = NULL;


    ZwClose(hd);

    PDRIVER_OBJECT drivercible = getdriverpointeur(&name);
    drivercibleglobal = drivercible;


    if (!drivercibleglobal) {
        return STATUS_NOT_FOUND;
    }
    if (drivercibleglobal) {
        KdPrint(("DriverObject = %p\n", drivercibleglobal));
    }
    UNICODE_STRING name2;
    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        driver->MajorFunction[i] = PassThrough;

    }
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Hookedioctl;
    driver->MajorFunction[IRP_MJ_READ] = Hookedread;
    driver->MajorFunction[IRP_MJ_WRITE] = Hookedwrite;

    RtlInitUnicodeString(&name2, L"\\Device\\filter");
    IoCreateDevice(driver, sizeof(DEVICE_EXTENSION), &name2, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &dev);
    DEVICE_EXTENSION* ext =
        (DEVICE_EXTENSION*)dev->DeviceExtension;
    ext->LowerDeviceObject = IoAttachDeviceToDeviceStack(dev, drivercibleglobal->DeviceObject);




    return STATUS_SUCCESS;
}

NTSTATUS PassThrough(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DEVICE_EXTENSION* ext = (DEVICE_EXTENSION*)DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(ext->LowerDeviceObject, Irp);
}

NTSTATUS routine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID context) {
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(context);
    ULONG len = (ULONG)Irp->IoStatus.Information;

    char* buf = (char*)Irp->AssociatedIrp.SystemBuffer;
    if (buf && len) {
        KdPrint(("contenu de la requete : %.*s", (int)len, buf));
    }
    return STATUS_CONTINUE_COMPLETION;

}

NTSTATUS Hookedioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    DEVICE_EXTENSION* ext = (DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, routine, NULL, TRUE, TRUE, TRUE);


    return IoCallDriver(ext->LowerDeviceObject, Irp);
}

NTSTATUS Hookedread(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    DEVICE_EXTENSION* ext = (DEVICE_EXTENSION*)DeviceObject->DeviceExtension;

    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

    KdPrint(("read taille  : %lu", stack->Parameters.Read.Length));

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp, routine, NULL, TRUE, TRUE, TRUE);

    return IoCallDriver(ext->LowerDeviceObject, Irp);
}
NTSTATUS Hookedwrite(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    DEVICE_EXTENSION* ext = (DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG lenght = stack->Parameters.Write.Length;
    PUCHAR buff = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
    KdPrint(("wrtie  taille : %lu\n", lenght));

    if (buff && lenght) {
        KdPrint(("write contenu : "));
        for (ULONG i = 0; i < lenght && i < 64; i++) {
            KdPrint(("%02X ", buff[i]));
        }

        KdPrint(("\n"));
    }

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(ext->LowerDeviceObject, Irp);
}

PDRIVER_OBJECT getdriverpointeur(PUNICODE_STRING objectname) {
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

    if (dev) {
        DEVICE_EXTENSION* ext = (DEVICE_EXTENSION*)dev->DeviceExtension;

        if (ext->LowerDeviceObject) {
            IoDetachDevice(ext->LowerDeviceObject);
        }


        IoDeleteDevice(dev);
        dev = nullptr;
    }

    KdPrint(("succes unload \n"));
}

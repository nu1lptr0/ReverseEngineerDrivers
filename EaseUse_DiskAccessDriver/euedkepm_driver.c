
NTSTATUS __stdcall DriverEntry(PDRIVER_OBJECT DriverObject, UNICODE_STRING *RegistryPath)
{
    SOME_CANARY_CHECK_FUNCTION();
  return Driver_Main(DriverObject, RegistryPath);
}

__int64 __fastcall Driver_Main(PDRIVER_OBJECT DriverObject, UNICODE_STRING *RegistryPath)
{
  int Disk; // ebx

  Disk = QueryDisk(RegistryPath);
  if ( Disk >= 0 )
  {
    Disk = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, 0, &DeviceObject);
    if ( Disk >= 0 )
    {
      Disk = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);
      if ( Disk >= 0 )
      {
        DeviceObject->Flags |= FILE_DEVICE_MULTI_UNC_PROVIDER;
        DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)DriverCreate;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)DriverClose;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)DriverIoControl;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP] = (PDRIVER_DISPATCH)DriverCleanup;
        DriverObject->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH)DriverRead;
        DriverObject->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH)DriverWrite;
        DriverObject->DriverUnload = (PDRIVER_UNLOAD)DriverUnload;
      }
      else
      {
        IoDeleteDevice(DeviceObject);
      }
    }
  }
  return (unsigned int)Disk;
}

__int64 __fastcall DriverCreate(_DEVICE_OBJECT *DeviceObject_1, IRP *Irp)
{
  _IO_STACK_LOCATION *CurrentStackLocation; // rsi
  unsigned int status; // ebx
  PFILE_OBJECT File; // rsi
  int v6; // eax
  _DEVICE_OBJECT *AttachedDeviceObject; // rdi
  bool IsDevice; // zf
  PDEVICE_OBJECT AttachedDeviceReference; // r12
  PDEVICE_OBJECT LowerDeviceObject; // rdi
  _DRIVER_OBJECT *DriverObject; // rcx
  ULONG Value; // [rsp+30h] [rbp-E8h] BYREF
  UNICODE_STRING String; // [rsp+38h] [rbp-E0h] BYREF
  PDEVICE_OBJECT DeviceObject; // [rsp+48h] [rbp-D0h] BYREF
  PFILE_OBJECT FileObject; // [rsp+50h] [rbp-C8h] BYREF
  struct _UNICODE_STRING DestinationString; // [rsp+58h] [rbp-C0h] BYREF
  WCHAR SourceString[64]; // [rsp+70h] [rbp-A8h] BYREF

  CurrentStackLocation = Irp->Tail.Overlay.CurrentStackLocation;
  Value = 0;
  String.Buffer = 0;
  if ( CurrentStackLocation && (File = CurrentStackLocation->FileObject) != 0 && File->FileName.Length > 1u )
  {
    String.Buffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, File->FileName.Length, 'rdse');
    if ( String.Buffer )
    {
      String.Length = File->FileName.Length - 2;
      String.MaximumLength = File->FileName.Length;
      memmove(String.Buffer, File->FileName.Buffer + 1, File->FileName.Length - 2);
      status = RtlUnicodeStringToInteger(&String, 10u, &Value);
      if ( !status )
      {
        if ( Value >= 0x64 )
          goto Invalid_parameter;
        DeviceObject = 0;
        FileObject = 0;
        memset(SourceString, 0, 120);
        SetDiskNumber(SourceString, 120, (wchar_t *)L"\\Device\\Harddisk%u\\Partition0", Value);
        if ( v6 )
          goto Invalid_parameter;
        RtlInitUnicodeString(&DestinationString, SourceString);
        if ( IoGetDeviceObjectPointer(&DestinationString, 0, &FileObject, &DeviceObject) )
          goto Invalid_parameter;
        ObfReferenceObject(DeviceObject);
        AttachedDeviceObject = DeviceObject;
        IsDevice = DeviceObject == 0;
        File->FsContext = FileObject;
        if ( IsDevice )
          goto Invalid_parameter;
        AttachedDeviceReference = IoGetAttachedDeviceReference(AttachedDeviceObject);
        if ( !AttachedDeviceReference )
          goto Invalid_parameter;
        ObfDereferenceObject(AttachedDeviceObject);
        RtlInitUnicodeString(&DestinationString, L"Disk");
        LowerDeviceObject = AttachedDeviceReference;
        do
        {
          DriverObject = LowerDeviceObject->DriverObject;
          if ( DriverObject )
          {
            if ( !RtlCompareUnicodeString(&DriverObject->DriverExtension->ServiceKeyName, &DestinationString, 1u) )
              break;
            if ( AttachedDeviceReference != LowerDeviceObject )
              ObfDereferenceObject(LowerDeviceObject);
          }
          LowerDeviceObject = IoGetLowerDeviceObject(LowerDeviceObject);
        }
        while ( LowerDeviceObject );
        if ( !LowerDeviceObject )
        {
Invalid_parameter:
          status = STATUS_INVALID_PARAMETER;
        }
        else
        {
          ObfDereferenceObject(AttachedDeviceReference);
          File->FsContext2 = LowerDeviceObject;
          Irp->IoStatus.Information = 0;
        }
      }
    }
    else
    {
      status = STATUS_INSUFFICIENT_RESOURCES;
    }
    if ( String.Buffer )
    {
      ExFreePoolWithTag(String.Buffer, 0);
      String.Buffer = 0;
      String.Length = 0;
      String.MaximumLength = 0;
    }
  }
  else
  {
    status = STATUS_INVALID_PARAMETER;
  }
  Irp->IoStatus.Status = status;
  IofCompleteRequest(Irp, IO_NO_INCREMENT);
  return status;
}

__int64 __fastcall DriverClose(_DEVICE_OBJECT *DeviceObject, IRP *Irp)
{
  _IO_STACK_LOCATION *CurrentStackLocation; // rdx
  PFILE_OBJECT FileObject; // rax
  PVOID FsContext2; // rcx
  PVOID FsContext; // rdi

  CurrentStackLocation = Irp->Tail.Overlay.CurrentStackLocation;
  if ( CurrentStackLocation )
  {
    FileObject = CurrentStackLocation->FileObject;
    if ( FileObject )
    {
      FsContext2 = FileObject->FsContext2;
      if ( FsContext2 )
      {
        FsContext = FileObject->FsContext;
        ObfDereferenceObject(FsContext2);
        if ( FsContext )
          ObfDereferenceObject(FsContext);
      }
    }
  }
  Irp->IoStatus.Information = 0;
  Irp->IoStatus.Status = STATUS_SUCCESS;
  IofCompleteRequest(Irp, IO_NO_INCREMENT);
  return STATUS_SUCCESS;
}

__int64 __fastcall DriverIoControl(_DEVICE_OBJECT *DeviceObject, IRP *Irp)
{
  _IO_STACK_LOCATION *DeviceIrp; // rbx
  PFILE_OBJECT FileObject; // rax
  _DEVICE_OBJECT *FsContext2; // rcx
  _DEVICE_OBJECT *AttachedDeviceReference; // rsi
  unsigned int Status; // ebx
  _IRP *OutputBuffer; // rbp
  IRP *AttacheddeviceIrp; // rax
  struct _IO_STATUS_BLOCK IoStatusBlock; // [rsp+50h] [rbp-38h] BYREF
  struct _KEVENT Event; // [rsp+60h] [rbp-28h] BYREF

  DeviceIrp = Irp->Tail.Overlay.CurrentStackLocation;
  if ( DeviceIrp
    && (FileObject = DeviceIrp->FileObject) != 0
    && (FsContext2 = (_DEVICE_OBJECT *)FileObject->FsContext2) != 0 )
  {
    AttachedDeviceReference = IoGetAttachedDeviceReference(FsContext2);
    if ( AttachedDeviceReference && (OutputBuffer = Irp->AssociatedIrp.MasterIrp) != 0 )
    {
      KeInitializeEvent(&Event, NotificationEvent, FALSE);
      AttacheddeviceIrp = IoBuildDeviceIoControlRequest(
                            DeviceIrp->Parameters.Read.ByteOffset.LowPart,
                            AttachedDeviceReference,
                            OutputBuffer,
                            DeviceIrp->Parameters.Create.Options,
                            OutputBuffer,
                            DeviceIrp->Parameters.Read.Length,
                            FALSE,
                            &Event,
                            &IoStatusBlock);
      if ( AttacheddeviceIrp )
      {
        Status = IofCallDriver(AttachedDeviceReference, AttacheddeviceIrp);
        if ( Status == STATUS_PENDING )
        {
          KeWaitForSingleObject(&Event, Executive, 0, FALSE, 0);
          Status = IoStatusBlock.Status;
        }
        Irp->IoStatus.Information = IoStatusBlock.Information;
      }
      else
      {
        Status = STATUS_INSUFFICIENT_RESOURCES;
      }
    }
    else
    {
      Status = STATUS_INVALID_PARAMETER;
    }
    if ( AttachedDeviceReference )
      ObDereferenceObjectDeferDelete(AttachedDeviceReference);
  }
  else
  {
    Status = STATUS_INVALID_PARAMETER;
  }
  Irp->IoStatus.Status = Status;
  IofCompleteRequest(Irp, IO_NO_INCREMENT);
  return Status;
}

_int64 __fastcall DriverCleanup(_DEVICE_OBJECT *DeviceObject, IRP *Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IofCompleteRequest(Irp, IO_NO_INCREMENT);
  return STATUS_SUCCESS;
}

__int64 __fastcall DriverRead(_DEVICE_OBJECT *DeviceObject, IRP *Irp)
{
  _IO_STACK_LOCATION *CurrentStackLocation; // rsi
  PFILE_OBJECT FileObject; // rax
  _DEVICE_OBJECT *FsContext2; // rbx
  PMDL MdlAddress; // rcx
  PVOID MappedSystemVa; // r8
  unsigned int Status; // ebx
  PIRP DeviceIrp; // rbp
  _IO_STACK_LOCATION *StackLocation; // r11
  struct _IO_STATUS_BLOCK IoStatusBlock; // [rsp+30h] [rbp-38h] BYREF
  struct _KEVENT Event; // [rsp+40h] [rbp-28h] BYREF
  union _LARGE_INTEGER Timeout; // [rsp+78h] [rbp+10h] BYREF

  CurrentStackLocation = Irp->Tail.Overlay.CurrentStackLocation;
  if ( !CurrentStackLocation )
    goto STATUS_INVALID_PARAMETER;
  FileObject = CurrentStackLocation->FileObject;
  if ( !FileObject )
    goto STATUS_INVALID_PARAMETER;
  FsContext2 = (_DEVICE_OBJECT *)FileObject->FsContext2;
  if ( !FsContext2 )
    goto STATUS_INVALID_PARAMETER;
  MdlAddress = Irp->MdlAddress;
  if ( (MdlAddress->MdlFlags & 5) != 0 )
    MappedSystemVa = MdlAddress->MappedSystemVa;
  else
    MappedSystemVa = MmMapLockedPagesSpecifyCache(MdlAddress, 0, MmCached, 0, FALSE, MDL_PARTIAL);
  if ( !MappedSystemVa )
    goto STATUS_INSUFFICIENT_RESOURCES;
  Timeout = CurrentStackLocation->Parameters.Read.ByteOffset;
  if ( Timeout.QuadPart < 0 )
  {
STATUS_INVALID_PARAMETER:
    Status = STATUS_INVALID_PARAMETER;
    goto STATUS_COMPLETE_REQUEST;
  }
  DeviceIrp = IoBuildAsynchronousFsdRequest(
                IRP_MJ_READ,
                FsContext2,
                MappedSystemVa,
                CurrentStackLocation->Parameters.Read.Length,
                &Timeout,
                &IoStatusBlock);
  if ( !DeviceIrp )
  {
STATUS_INSUFFICIENT_RESOURCES:
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto STATUS_COMPLETE_REQUEST;
  }
  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  // IoGetNextIrpStackLocation(StackLocation)
  StackLocation = DeviceIrp->Tail.Overlay.CurrentStackLocation;
  StackLocation[-1].CompletionRoutine = (PIO_COMPLETION_ROUTINE)IoGetNextIrpStackLocation;
  StackLocation[-1].Control = -32;
  StackLocation[-1].Context = &Event;

  Status = IofCallDriver(FsContext2, DeviceIrp);
  if ( Status == STATUS_PENDING )
  {
    KeWaitForSingleObject(&Event, Executive, 0, FALSE, 0);
    Status = IoStatusBlock.Status;
  }
  if ( !Status )
    Irp->IoStatus.Information = CurrentStackLocation->Parameters.Read.Length << 9;
STATUS_COMPLETE_REQUEST:
  Irp->IoStatus.Status = Status;
  IofCompleteRequest(Irp, IO_NO_INCREMENT);
  return Status;
}

__int64 __fastcall DriverWrite(_DEVICE_OBJECT *DeviceObject, IRP *Irp)
{
  _IO_STACK_LOCATION *CurrentStackLocation; // rsi
  PFILE_OBJECT FileObject; // rax
  _DEVICE_OBJECT *FsContext2; // rbx
  PMDL MdlAddress; // rcx
  PVOID MappedSystemVa; // r8
  unsigned int Status; // ebx
  PIRP DeviceIrp; // rbp
  _IO_STACK_LOCATION *StackLocation; // r11
  struct _IO_STATUS_BLOCK IoStatusBlock; // [rsp+30h] [rbp-38h] BYREF
  struct _KEVENT Event; // [rsp+40h] [rbp-28h] BYREF
  union _LARGE_INTEGER Timeout; // [rsp+78h] [rbp+10h] BYREF

  CurrentStackLocation = Irp->Tail.Overlay.CurrentStackLocation;
  if ( !CurrentStackLocation )
    goto STATUS_INVALID_PARAMETER;
  FileObject = CurrentStackLocation->FileObject;
  if ( !FileObject )
    goto STATUS_INVALID_PARAMETER;
  FsContext2 = (_DEVICE_OBJECT *)FileObject->FsContext2;
  if ( !FsContext2 )
    goto STATUS_INVALID_PARAMETER;
  MdlAddress = Irp->MdlAddress;
  if ( (MdlAddress->MdlFlags & 5) != 0 )
    MappedSystemVa = MdlAddress->MappedSystemVa;
  else
    MappedSystemVa = MmMapLockedPagesSpecifyCache(MdlAddress, 0, MmCached, 0, 0, 0x10u);
  if ( !MappedSystemVa )
    goto STATUS_INSUFFICIENT_RESOURCES;
  Timeout = CurrentStackLocation->Parameters.Read.ByteOffset;
  if ( Timeout.QuadPart < 0 )
  {
STATUS_INVALID_PARAMETER:
    Status = STATUS_INVALID_PARAMETER;
    goto IO_COMPLETE_REQUEST;
  }
  DeviceIrp = IoBuildAsynchronousFsdRequest(
                IRP_MJ_WRITE,
                FsContext2,
                MappedSystemVa,
                CurrentStackLocation->Parameters.Read.Length,
                &Timeout,
                &IoStatusBlock);
  if ( !DeviceIrp )
  {
STATUS_INSUFFICIENT_RESOURCES:
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto IO_COMPLETE_REQUEST;
  }
  KeInitializeEvent(&Event, NotificationEvent, 0);
  // IoGetNextIrpStackLocation(StackLocation)
  StackLocation = DeviceIrp->Tail.Overlay.CurrentStackLocation;
  StackLocation[-1].CompletionRoutine = (PIO_COMPLETION_ROUTINE)IoGetNextIrpStackLocation;
  StackLocation[-1].Control = -32;
  StackLocation[-1].Context = &Event;
  Status = IofCallDriver(FsContext2, DeviceIrp);
  if ( Status == 259 )
  {
    KeWaitForSingleObject(&Event, Executive, 0, FALSE, 0);
    Status = IoStatusBlock.Status;
  }
  if ( !Status )
    Irp->IoStatus.Information = CurrentStackLocation->Parameters.Read.Length << 9;
IO_COMPLETE_REQUEST:
  Irp->IoStatus.Status = Status;
  IofCompleteRequest(Irp, IO_NO_INCREMENT);
  return Status;
}

void DriverUnload()
{
  IoDeleteSymbolicLink(&SymbolicLinkName);
  IoDeleteDevice(DeviceObject);
  RtlFreeUnicodeString(&DeviceName);
  RtlFreeUnicodeString(&SymbolicLinkName);
}
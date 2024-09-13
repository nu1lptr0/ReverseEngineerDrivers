# EASEUSE PARTITION MANAGER

### <u> DISK ACCESS DRIVER</u>
**EUEDKEPM.sys** is the Disk Access Driver for the Easeuse partition manager software.

#### DriverEntry
- This a WDM windows driver. It starts a `DriverEntry` function which does some basic security checks and then enters the `DriverMain` function .
- This function has a `QueryDisk` function and the basic windows driver `IoCreateDevice` and `IoCreateSymbolicLink` windows API for driver Creation and creating Symbolic Link.

- Initialized 6 Major Functions and a `DriverUnload` function and device flag as `FILE_DEVICE_MULTI_UNC_PROVIDER`.
```C
    DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)DriverCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)DriverClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)DriverIoControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = (PDRIVER_DISPATCH)DriverCleanup;
    DriverObject->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH)DriverRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH)DriverWrite;
    DriverObject->DriverUnload = (PDRIVER_UNLOAD)DriverUnload;
```
#### QueryDisk
- It takes the `RegistryPath` as first argument and create a `Attributes` from it to open the key corresponding to using the `ZwopenKey` API.
- Then , It Queries the KeyValue using `ZwQueryValueKey` as `KeyValuePartialInformation` and stores the value in a `KEY_VALUE_PARTIAL_INFORMATION` structure allocted using `ExAllocatePool` API in NonPagedPool.
- Then , It zeroes out the global buffer `DeviceName` and `SymbolicLinkName` for storing the Created device name.

#### DriverCreate
- This function creates a Attached device to `\\Device\\Harddisk%u\\Partition0` (u is set accordingly) and then compares the service KeyName with `Disk` and the intializes it as `LowerdeviceObject`.

#### DriverIoControl
- It uses the `IoBuildDeviceIoControlRequest` API to set up an IRP for asynchronous communication.
- Then uses the `IofCallDriver` API to send the IRP to the Attached Device.

#### DriverRead & DriverWrite
- Both function uses to allocate virtual memory using the `MmMapLockedPagesSpecifyCache` to allocate `MmCached` memory to be used for read and write .
- Then ,`IoBuildAsynchronousFsRequest` API is uses to create IRP with the Lower Device object on this previous Allocated memory using Major function as `IRP_MJ_READ` and `IRP_MJ_WRITE` vice versa.
- Then , at last `IofCallDriver` function is used send the IRP to the Lower device Object.


#### DriverClose & DriverCleanup & DriverUnload
- `DriverClose` closes the file Object and set `STATUS_SUCCESS`.
- `DriverUnload` frees the created `Symboliclink` and `DeviceName` , then deletes the created device.
- `DriverCleanUp` sets the `STATUS_SUCCESS` and complete the IRP request.


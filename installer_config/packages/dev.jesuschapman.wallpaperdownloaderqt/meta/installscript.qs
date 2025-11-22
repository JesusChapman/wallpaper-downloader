function Component()
{
    // Constructor por defecto
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        
        var shortcutName = "Wallpaper downloader";

        // 2. Crear acceso directo en el Menú Inicio
        component.addOperation("CreateShortcut", 
                               "@TargetDir@/wallpaper_downloader.exe", 
                               "@StartMenuDir@/" + shortcutName + ".lnk",
                               "workingDirectory=@TargetDir@",
                               "iconPath=@TargetDir@/wallpaper_downloader.exe", "iconId=0",
                               "description=Un cliente para descargar fondos de escritorio que usa la api de wallhaven y Qt widgets framework");

        // 3. Crear acceso directo en el Escritorio
        component.addOperation("CreateShortcut", 
                               "@TargetDir@/wallpaper_downloader.exe", 
                               "@DesktopDir@/" + shortcutName + ".lnk",
                               "workingDirectory=@TargetDir@",
                               "iconPath=@TargetDir@/wallpaper_downloader.exe", "iconId=0",
                               "description=Un cliente para descargar fondos de escritorio que usa la api de wallhaven y Qt widgets framework");
    }
}
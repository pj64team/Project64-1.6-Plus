Imports System.Diagnostics
Imports System.IO
Public Class Main

    Private Sub Button_Submit_Click(sender As System.Object, e As System.EventArgs) Handles Button_Uninstall.Click
        Dim proc As Process = Nothing
        Try
            proc = New Process()
            IO.File.WriteAllText("Uninstall.Bat", My.Resources.Uninstall_Bat)
            proc.StartInfo.FileName = "Uninstall.Bat"
            IO.File.WriteAllText("Uninstall.Reg", My.Resources.Uninstall_Reg)
            proc.StartInfo.FileName = "Uninstall.Reg"
            proc.StartInfo.CreateNoWindow = False
            proc.Start()
            Dim windowStyle As Integer : windowStyle = 0
            proc.WaitForExit()
        Catch ex As Exception
            Console.WriteLine(ex.StackTrace.ToString())
        End Try
        File.Delete("Uninstall.Bat")
        File.Delete("Uninstall.Reg")
        Close()
    End Sub

    Private Sub Button_Close_Click(sender As System.Object, e As System.EventArgs) Handles Button_Close.Click
        File.Delete("Uninstall.Bat")
        File.Delete("Uninstall.Reg")
        Close()
    End Sub

End Class



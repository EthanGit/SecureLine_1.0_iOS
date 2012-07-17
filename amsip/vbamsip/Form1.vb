Public Class Form1
    Inherits System.Windows.Forms.Form

    Private Declare Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As Long)
    Private Declare Function GetDC Lib "user32" (ByVal hWnd As IntPtr) As IntPtr
    Private pbVideo_dc As IntPtr

    Private myapi As amsipapi = New amsipapi
    Private interval_for_stat As Integer
    Private appClosing As Boolean
    Private amsipRunning As Boolean
    Private amsipTimerRunning As Boolean
    Private amsipProxy As String
    Private amsipUsername As String
    Private amsipPassword As String

    Private online_ico As System.Drawing.Image
    Private hold_ico As System.Drawing.Image
    Private video_ico As System.Drawing.Image

    Private away_ico As System.Drawing.Image
    Private bifm_ico As System.Drawing.Image
    Private busy_ico As System.Drawing.Image
    Private closed_ico As System.Drawing.Image
    Private wfa_ico As System.Drawing.Image

#Region " Code généré par le Concepteur Windows Form "

    Friend WithEvents cIce As System.Windows.Forms.CheckBox
    Friend WithEvents pbVideo As System.Windows.Forms.PictureBox
    Friend WithEvents cbPlayDevice As System.Windows.Forms.ComboBox
    Friend WithEvents cbRecordDevice As System.Windows.Forms.ComboBox
    Friend WithEvents cbDownloadB As System.Windows.Forms.ComboBox
    Friend WithEvents cbUploadB As System.Windows.Forms.ComboBox
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents lC_callinfo1 As System.Windows.Forms.Label
    Friend WithEvents bC1_1 As System.Windows.Forms.PictureBox
    Friend WithEvents bC2_1 As System.Windows.Forms.PictureBox
    Friend WithEvents lC_callinfo2 As System.Windows.Forms.Label
    Friend WithEvents bC4_1 As System.Windows.Forms.PictureBox
    Friend WithEvents lC_callinfo4 As System.Windows.Forms.Label
    Friend WithEvents bC3_1 As System.Windows.Forms.PictureBox
    Friend WithEvents lC_callinfo3 As System.Windows.Forms.Label
    Friend WithEvents bC1_2 As System.Windows.Forms.PictureBox
    Friend WithEvents bC1_3 As System.Windows.Forms.PictureBox
    Friend WithEvents bC2_3 As System.Windows.Forms.PictureBox
    Friend WithEvents bC2_2 As System.Windows.Forms.PictureBox
    Friend WithEvents bC3_3 As System.Windows.Forms.PictureBox
    Friend WithEvents bC3_2 As System.Windows.Forms.PictureBox
    Friend WithEvents bC4_3 As System.Windows.Forms.PictureBox
    Friend WithEvents bC4_2 As System.Windows.Forms.PictureBox
    Friend WithEvents ttStopCall As System.Windows.Forms.ToolTip
    Friend WithEvents ttRTPstat As System.Windows.Forms.ToolTip
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents cbTransferList As System.Windows.Forms.ComboBox
    Friend WithEvents cbTargetList As System.Windows.Forms.ComboBox
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents Button1 As System.Windows.Forms.Button
    Friend WithEvents pbVideoPreview As System.Windows.Forms.PictureBox
    Friend WithEvents cbWebcam As System.Windows.Forms.ComboBox
    Friend WithEvents Label6 As System.Windows.Forms.Label

    Public Sub New()
        MyBase.New()

        'Cet appel est requis par le Concepteur Windows Form.
        InitializeComponent()

        'Ajoutez une initialisation quelconque après l'appel InitializeComponent()
        appClosing = False
        amsipRunning = False
        amsipTimerRunning = False
        interval_for_stat = 0
        'myapi = New amsipapi

        Me.pbVideo_dc = GetDC(Me.pbVideo.Handle)

        online_ico = CType(My.Resources.icons.sip_online, System.Drawing.Image)
        busy_ico = CType(My.Resources.icons.sip_busy, System.Drawing.Image)
        hold_ico = CType(My.Resources.icons.sip_hold, System.Drawing.Image)
        video_ico = CType(My.Resources.icons.sip_video, System.Drawing.Image)

        closed_ico = CType(My.Resources.icons.sip_closed, System.Drawing.Image)
        away_ico = CType(My.Resources.icons.sip_away, System.Drawing.Image)
        bifm_ico = CType(My.Resources.icons.sip_bifm, System.Drawing.Image)
        wfa_ico = CType(My.Resources.icons.sip_wfa, System.Drawing.Image)

        Me.bC1_1.Image = online_ico
        Me.bC2_1.Image = online_ico
        Me.bC3_1.Image = online_ico
        Me.bC4_1.Image = online_ico

        Dim Ret As Integer
        Ret = myapi.API_load_am_config("amsip_cfg.xml")
        If (Ret >= 0) Then
            tProxy.Text = myapi.local_config.domain_realm
            tUsername.Text = myapi.local_config.username
            tPassword.Text = myapi.local_config.password

            If (myapi.local_config.AEC_on = True) Then
                cAec.Checked = True
            Else
                cAec.Checked = False
            End If
            If (myapi.local_config.stun_server = Nothing) Then
                cIce.Checked = False
            Else
                cIce.Checked = True
            End If
        End If

    End Sub

    'La méthode substituée Dispose du formulaire pour nettoyer la liste des composants.
    Protected Overloads Overrides Sub Dispose(ByVal disposing As Boolean)
        If disposing Then
            If Not (components Is Nothing) Then
                components.Dispose()
            End If
        End If
        MyBase.Dispose(disposing)
    End Sub

    'Requis par le Concepteur Windows Form
    Private components As System.ComponentModel.IContainer

    'REMARQUE : la procédure suivante est requise par le Concepteur Windows Form
    'Elle peut être modifiée en utilisant le Concepteur Windows Form.  
    'Ne la modifiez pas en utilisant l'éditeur de code.
    Friend WithEvents bStartCall As System.Windows.Forms.Button
    Friend WithEvents bInitAmsip As System.Windows.Forms.Button
    Friend WithEvents bRegister As System.Windows.Forms.Button
    Friend WithEvents lUsername As System.Windows.Forms.Label
    Friend WithEvents tUsername As System.Windows.Forms.TextBox
    Friend WithEvents tPassword As System.Windows.Forms.TextBox
    Friend WithEvents lPassword As System.Windows.Forms.Label
    Friend WithEvents lHelloWorld As System.Windows.Forms.Label
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents TimerAmsip As System.Windows.Forms.Timer
    Friend WithEvents tProxy As System.Windows.Forms.TextBox
    Friend WithEvents lProxy As System.Windows.Forms.Label
    Friend WithEvents tCallee As System.Windows.Forms.TextBox
    Friend WithEvents lCallee As System.Windows.Forms.Label
    Friend WithEvents PictureBox1 As System.Windows.Forms.PictureBox
    Friend WithEvents lOnlineStatus As System.Windows.Forms.Label
    Friend WithEvents lCallStatus As System.Windows.Forms.Label
    Friend WithEvents bExit As System.Windows.Forms.Button
    Friend WithEvents cAec As System.Windows.Forms.CheckBox
    <System.Diagnostics.DebuggerStepThrough()> Private Sub InitializeComponent()
        Me.components = New System.ComponentModel.Container()
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(Form1))
        Me.bStartCall = New System.Windows.Forms.Button()
        Me.bInitAmsip = New System.Windows.Forms.Button()
        Me.bRegister = New System.Windows.Forms.Button()
        Me.lUsername = New System.Windows.Forms.Label()
        Me.tUsername = New System.Windows.Forms.TextBox()
        Me.tPassword = New System.Windows.Forms.TextBox()
        Me.lPassword = New System.Windows.Forms.Label()
        Me.lHelloWorld = New System.Windows.Forms.Label()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.TimerAmsip = New System.Windows.Forms.Timer(Me.components)
        Me.tProxy = New System.Windows.Forms.TextBox()
        Me.lProxy = New System.Windows.Forms.Label()
        Me.tCallee = New System.Windows.Forms.TextBox()
        Me.lCallee = New System.Windows.Forms.Label()
        Me.lOnlineStatus = New System.Windows.Forms.Label()
        Me.lCallStatus = New System.Windows.Forms.Label()
        Me.cAec = New System.Windows.Forms.CheckBox()
        Me.bExit = New System.Windows.Forms.Button()
        Me.cIce = New System.Windows.Forms.CheckBox()
        Me.cbPlayDevice = New System.Windows.Forms.ComboBox()
        Me.cbRecordDevice = New System.Windows.Forms.ComboBox()
        Me.cbDownloadB = New System.Windows.Forms.ComboBox()
        Me.cbUploadB = New System.Windows.Forms.ComboBox()
        Me.Label5 = New System.Windows.Forms.Label()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.lC_callinfo1 = New System.Windows.Forms.Label()
        Me.lC_callinfo2 = New System.Windows.Forms.Label()
        Me.lC_callinfo4 = New System.Windows.Forms.Label()
        Me.lC_callinfo3 = New System.Windows.Forms.Label()
        Me.ttStopCall = New System.Windows.Forms.ToolTip(Me.components)
        Me.bC4_3 = New System.Windows.Forms.PictureBox()
        Me.bC4_2 = New System.Windows.Forms.PictureBox()
        Me.bC3_3 = New System.Windows.Forms.PictureBox()
        Me.bC3_2 = New System.Windows.Forms.PictureBox()
        Me.bC2_3 = New System.Windows.Forms.PictureBox()
        Me.bC2_2 = New System.Windows.Forms.PictureBox()
        Me.bC1_3 = New System.Windows.Forms.PictureBox()
        Me.bC1_2 = New System.Windows.Forms.PictureBox()
        Me.bC4_1 = New System.Windows.Forms.PictureBox()
        Me.bC3_1 = New System.Windows.Forms.PictureBox()
        Me.bC2_1 = New System.Windows.Forms.PictureBox()
        Me.bC1_1 = New System.Windows.Forms.PictureBox()
        Me.pbVideo = New System.Windows.Forms.PictureBox()
        Me.PictureBox1 = New System.Windows.Forms.PictureBox()
        Me.ttRTPstat = New System.Windows.Forms.ToolTip(Me.components)
        Me.Label2 = New System.Windows.Forms.Label()
        Me.cbTransferList = New System.Windows.Forms.ComboBox()
        Me.cbTargetList = New System.Windows.Forms.ComboBox()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.Button1 = New System.Windows.Forms.Button()
        Me.pbVideoPreview = New System.Windows.Forms.PictureBox()
        Me.cbWebcam = New System.Windows.Forms.ComboBox()
        CType(Me.bC4_3, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC4_2, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC3_3, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC3_2, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC2_3, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC2_2, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC1_3, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC1_2, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC4_1, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC3_1, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC2_1, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.bC1_1, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.pbVideo, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).BeginInit()
        CType(Me.pbVideoPreview, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'bStartCall
        '
        Me.bStartCall.Enabled = False
        Me.bStartCall.Location = New System.Drawing.Point(16, 272)
        Me.bStartCall.Name = "bStartCall"
        Me.bStartCall.Size = New System.Drawing.Size(262, 23)
        Me.bStartCall.TabIndex = 8
        Me.bStartCall.Text = "StartCall"
        '
        'bInitAmsip
        '
        Me.bInitAmsip.Location = New System.Drawing.Point(16, 187)
        Me.bInitAmsip.Name = "bInitAmsip"
        Me.bInitAmsip.Size = New System.Drawing.Size(87, 23)
        Me.bInitAmsip.TabIndex = 4
        Me.bInitAmsip.Text = "InitAmsip"
        '
        'bRegister
        '
        Me.bRegister.Enabled = False
        Me.bRegister.Location = New System.Drawing.Point(103, 187)
        Me.bRegister.Name = "bRegister"
        Me.bRegister.Size = New System.Drawing.Size(87, 23)
        Me.bRegister.TabIndex = 5
        Me.bRegister.Text = "Register"
        '
        'lUsername
        '
        Me.lUsername.Location = New System.Drawing.Point(16, 66)
        Me.lUsername.Name = "lUsername"
        Me.lUsername.Size = New System.Drawing.Size(64, 23)
        Me.lUsername.TabIndex = 4
        Me.lUsername.Text = "Username"
        '
        'tUsername
        '
        Me.tUsername.Location = New System.Drawing.Point(88, 66)
        Me.tUsername.Name = "tUsername"
        Me.tUsername.Size = New System.Drawing.Size(88, 20)
        Me.tUsername.TabIndex = 2
        Me.tUsername.Text = "test1"
        '
        'tPassword
        '
        Me.tPassword.Location = New System.Drawing.Point(88, 86)
        Me.tPassword.Name = "tPassword"
        Me.tPassword.Size = New System.Drawing.Size(88, 20)
        Me.tPassword.TabIndex = 3
        Me.tPassword.Text = "secret"
        '
        'lPassword
        '
        Me.lPassword.Location = New System.Drawing.Point(16, 86)
        Me.lPassword.Name = "lPassword"
        Me.lPassword.Size = New System.Drawing.Size(64, 23)
        Me.lPassword.TabIndex = 6
        Me.lPassword.Text = "Password"
        '
        'lHelloWorld
        '
        Me.lHelloWorld.Location = New System.Drawing.Point(54, 0)
        Me.lHelloWorld.Name = "lHelloWorld"
        Me.lHelloWorld.Size = New System.Drawing.Size(226, 23)
        Me.lHelloWorld.TabIndex = 8
        Me.lHelloWorld.Text = "This is a test application for amsip (4.6.0)"
        Me.lHelloWorld.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'Label1
        '
        Me.Label1.Location = New System.Drawing.Point(104, 23)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(176, 23)
        Me.Label1.TabIndex = 9
        Me.Label1.Text = "Provide your account information"
        Me.Label1.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'TimerAmsip
        '
        Me.TimerAmsip.Interval = 20
        '
        'tProxy
        '
        Me.tProxy.Location = New System.Drawing.Point(88, 46)
        Me.tProxy.Name = "tProxy"
        Me.tProxy.Size = New System.Drawing.Size(192, 20)
        Me.tProxy.TabIndex = 1
        Me.tProxy.Text = "sip.antisip.com"
        '
        'lProxy
        '
        Me.lProxy.Location = New System.Drawing.Point(16, 46)
        Me.lProxy.Name = "lProxy"
        Me.lProxy.Size = New System.Drawing.Size(64, 23)
        Me.lProxy.TabIndex = 10
        Me.lProxy.Text = "Proxy"
        '
        'tCallee
        '
        Me.tCallee.Location = New System.Drawing.Point(86, 248)
        Me.tCallee.Name = "tCallee"
        Me.tCallee.Size = New System.Drawing.Size(192, 20)
        Me.tCallee.TabIndex = 7
        Me.tCallee.Text = "sip:welcome@sip.antisip.com"
        '
        'lCallee
        '
        Me.lCallee.Location = New System.Drawing.Point(16, 248)
        Me.lCallee.Name = "lCallee"
        Me.lCallee.Size = New System.Drawing.Size(64, 23)
        Me.lCallee.TabIndex = 12
        Me.lCallee.Text = "Callee"
        '
        'lOnlineStatus
        '
        Me.lOnlineStatus.BackColor = System.Drawing.Color.White
        Me.lOnlineStatus.Location = New System.Drawing.Point(16, 217)
        Me.lOnlineStatus.Name = "lOnlineStatus"
        Me.lOnlineStatus.Size = New System.Drawing.Size(264, 23)
        Me.lOnlineStatus.TabIndex = 14
        Me.lOnlineStatus.Text = "Status: Not registered"
        Me.lOnlineStatus.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'lCallStatus
        '
        Me.lCallStatus.BackColor = System.Drawing.Color.White
        Me.lCallStatus.Location = New System.Drawing.Point(16, 326)
        Me.lCallStatus.Name = "lCallStatus"
        Me.lCallStatus.Size = New System.Drawing.Size(265, 23)
        Me.lCallStatus.TabIndex = 15
        Me.lCallStatus.Text = "Status: Not in call"
        Me.lCallStatus.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'cAec
        '
        Me.cAec.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.cAec.Location = New System.Drawing.Point(176, 66)
        Me.cAec.Name = "cAec"
        Me.cAec.Size = New System.Drawing.Size(104, 20)
        Me.cAec.TabIndex = 16
        Me.cAec.Text = "aec"
        Me.cAec.TextAlign = System.Drawing.ContentAlignment.MiddleRight
        '
        'bExit
        '
        Me.bExit.Location = New System.Drawing.Point(190, 187)
        Me.bExit.Name = "bExit"
        Me.bExit.Size = New System.Drawing.Size(87, 23)
        Me.bExit.TabIndex = 6
        Me.bExit.Text = "Exit"
        '
        'cIce
        '
        Me.cIce.CheckAlign = System.Drawing.ContentAlignment.MiddleRight
        Me.cIce.Checked = True
        Me.cIce.CheckState = System.Windows.Forms.CheckState.Checked
        Me.cIce.Location = New System.Drawing.Point(176, 86)
        Me.cIce.Name = "cIce"
        Me.cIce.Size = New System.Drawing.Size(104, 20)
        Me.cIce.TabIndex = 17
        Me.cIce.Text = "ICE"
        Me.cIce.TextAlign = System.Drawing.ContentAlignment.MiddleRight
        '
        'cbPlayDevice
        '
        Me.cbPlayDevice.Enabled = False
        Me.cbPlayDevice.FormattingEnabled = True
        Me.cbPlayDevice.Location = New System.Drawing.Point(16, 109)
        Me.cbPlayDevice.Name = "cbPlayDevice"
        Me.cbPlayDevice.Size = New System.Drawing.Size(140, 21)
        Me.cbPlayDevice.TabIndex = 59
        Me.cbPlayDevice.Text = "Default Playing Device"
        '
        'cbRecordDevice
        '
        Me.cbRecordDevice.Enabled = False
        Me.cbRecordDevice.FormattingEnabled = True
        Me.cbRecordDevice.Location = New System.Drawing.Point(16, 132)
        Me.cbRecordDevice.Name = "cbRecordDevice"
        Me.cbRecordDevice.Size = New System.Drawing.Size(140, 21)
        Me.cbRecordDevice.TabIndex = 60
        Me.cbRecordDevice.Text = "Default Recording Device"
        '
        'cbDownloadB
        '
        Me.cbDownloadB.FormattingEnabled = True
        Me.cbDownloadB.Items.AddRange(New Object() {"128k", "256k", "512k", "740k"})
        Me.cbDownloadB.Location = New System.Drawing.Point(223, 109)
        Me.cbDownloadB.Name = "cbDownloadB"
        Me.cbDownloadB.Size = New System.Drawing.Size(58, 21)
        Me.cbDownloadB.TabIndex = 61
        Me.cbDownloadB.Text = "512k"
        '
        'cbUploadB
        '
        Me.cbUploadB.FormattingEnabled = True
        Me.cbUploadB.Items.AddRange(New Object() {"128k", "256k", "512k", "740k"})
        Me.cbUploadB.Location = New System.Drawing.Point(223, 129)
        Me.cbUploadB.Name = "cbUploadB"
        Me.cbUploadB.Size = New System.Drawing.Size(58, 21)
        Me.cbUploadB.TabIndex = 62
        Me.cbUploadB.Text = "256k"
        '
        'Label5
        '
        Me.Label5.Location = New System.Drawing.Point(156, 109)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(64, 23)
        Me.Label5.TabIndex = 63
        Me.Label5.Text = "Downld BW"
        '
        'Label6
        '
        Me.Label6.Location = New System.Drawing.Point(156, 132)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(64, 23)
        Me.Label6.TabIndex = 64
        Me.Label6.Text = "Upload BW"
        '
        'lC_callinfo1
        '
        Me.lC_callinfo1.BackColor = System.Drawing.Color.White
        Me.lC_callinfo1.Location = New System.Drawing.Point(376, 46)
        Me.lC_callinfo1.Name = "lC_callinfo1"
        Me.lC_callinfo1.Size = New System.Drawing.Size(220, 25)
        Me.lC_callinfo1.TabIndex = 65
        Me.lC_callinfo1.Text = "Status: No Call Pending"
        Me.lC_callinfo1.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'lC_callinfo2
        '
        Me.lC_callinfo2.BackColor = System.Drawing.Color.White
        Me.lC_callinfo2.Location = New System.Drawing.Point(376, 71)
        Me.lC_callinfo2.Name = "lC_callinfo2"
        Me.lC_callinfo2.Size = New System.Drawing.Size(220, 25)
        Me.lC_callinfo2.TabIndex = 68
        Me.lC_callinfo2.Text = "Status: No Call Pending"
        Me.lC_callinfo2.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'lC_callinfo4
        '
        Me.lC_callinfo4.BackColor = System.Drawing.Color.White
        Me.lC_callinfo4.Location = New System.Drawing.Point(376, 121)
        Me.lC_callinfo4.Name = "lC_callinfo4"
        Me.lC_callinfo4.Size = New System.Drawing.Size(220, 25)
        Me.lC_callinfo4.TabIndex = 74
        Me.lC_callinfo4.Text = "Status: No Call Pending"
        Me.lC_callinfo4.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'lC_callinfo3
        '
        Me.lC_callinfo3.BackColor = System.Drawing.Color.White
        Me.lC_callinfo3.Location = New System.Drawing.Point(376, 96)
        Me.lC_callinfo3.Name = "lC_callinfo3"
        Me.lC_callinfo3.Size = New System.Drawing.Size(220, 25)
        Me.lC_callinfo3.TabIndex = 71
        Me.lC_callinfo3.Text = "Status: No Call Pending"
        Me.lC_callinfo3.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'ttStopCall
        '
        Me.ttStopCall.ToolTipTitle = "Click Here!"
        '
        'bC4_3
        '
        Me.bC4_3.Location = New System.Drawing.Point(345, 121)
        Me.bC4_3.Name = "bC4_3"
        Me.bC4_3.Size = New System.Drawing.Size(25, 25)
        Me.bC4_3.TabIndex = 83
        Me.bC4_3.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC4_3, "to start video")
        '
        'bC4_2
        '
        Me.bC4_2.Location = New System.Drawing.Point(320, 121)
        Me.bC4_2.Name = "bC4_2"
        Me.bC4_2.Size = New System.Drawing.Size(25, 25)
        Me.bC4_2.TabIndex = 82
        Me.bC4_2.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC4_2, "to put your correspondant on hold call")
        '
        'bC3_3
        '
        Me.bC3_3.Location = New System.Drawing.Point(345, 96)
        Me.bC3_3.Name = "bC3_3"
        Me.bC3_3.Size = New System.Drawing.Size(25, 25)
        Me.bC3_3.TabIndex = 81
        Me.bC3_3.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC3_3, "to start video")
        '
        'bC3_2
        '
        Me.bC3_2.Location = New System.Drawing.Point(320, 96)
        Me.bC3_2.Name = "bC3_2"
        Me.bC3_2.Size = New System.Drawing.Size(25, 25)
        Me.bC3_2.TabIndex = 80
        Me.bC3_2.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC3_2, "to put your correspondant on hold call")
        '
        'bC2_3
        '
        Me.bC2_3.Location = New System.Drawing.Point(345, 71)
        Me.bC2_3.Name = "bC2_3"
        Me.bC2_3.Size = New System.Drawing.Size(25, 25)
        Me.bC2_3.TabIndex = 79
        Me.bC2_3.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC2_3, "to start video")
        '
        'bC2_2
        '
        Me.bC2_2.Location = New System.Drawing.Point(320, 71)
        Me.bC2_2.Name = "bC2_2"
        Me.bC2_2.Size = New System.Drawing.Size(25, 25)
        Me.bC2_2.TabIndex = 78
        Me.bC2_2.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC2_2, "to put your correspondant on hold call")
        '
        'bC1_3
        '
        Me.bC1_3.Location = New System.Drawing.Point(345, 46)
        Me.bC1_3.Name = "bC1_3"
        Me.bC1_3.Size = New System.Drawing.Size(25, 25)
        Me.bC1_3.TabIndex = 77
        Me.bC1_3.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC1_3, "to start video")
        '
        'bC1_2
        '
        Me.bC1_2.Location = New System.Drawing.Point(320, 46)
        Me.bC1_2.Name = "bC1_2"
        Me.bC1_2.Size = New System.Drawing.Size(25, 25)
        Me.bC1_2.TabIndex = 76
        Me.bC1_2.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC1_2, "to put your correspondant on hold call")
        '
        'bC4_1
        '
        Me.bC4_1.Location = New System.Drawing.Point(295, 121)
        Me.bC4_1.Name = "bC4_1"
        Me.bC4_1.Size = New System.Drawing.Size(25, 25)
        Me.bC4_1.TabIndex = 75
        Me.bC4_1.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC4_1, "to stop call")
        '
        'bC3_1
        '
        Me.bC3_1.Location = New System.Drawing.Point(295, 96)
        Me.bC3_1.Name = "bC3_1"
        Me.bC3_1.Size = New System.Drawing.Size(25, 25)
        Me.bC3_1.TabIndex = 72
        Me.bC3_1.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC3_1, "to stop call")
        '
        'bC2_1
        '
        Me.bC2_1.Location = New System.Drawing.Point(295, 71)
        Me.bC2_1.Name = "bC2_1"
        Me.bC2_1.Size = New System.Drawing.Size(25, 25)
        Me.bC2_1.TabIndex = 69
        Me.bC2_1.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC2_1, "to stop call")
        '
        'bC1_1
        '
        Me.bC1_1.Location = New System.Drawing.Point(295, 46)
        Me.bC1_1.Name = "bC1_1"
        Me.bC1_1.Size = New System.Drawing.Size(25, 25)
        Me.bC1_1.TabIndex = 66
        Me.bC1_1.TabStop = False
        Me.ttStopCall.SetToolTip(Me.bC1_1, "to stop call")
        '
        'pbVideo
        '
        Me.pbVideo.Location = New System.Drawing.Point(615, 22)
        Me.pbVideo.Name = "pbVideo"
        Me.pbVideo.Size = New System.Drawing.Size(352, 288)
        Me.pbVideo.TabIndex = 58
        Me.pbVideo.TabStop = False
        '
        'PictureBox1
        '
        Me.PictureBox1.Image = CType(resources.GetObject("PictureBox1.Image"), System.Drawing.Image)
        Me.PictureBox1.Location = New System.Drawing.Point(16, 8)
        Me.PictureBox1.Name = "PictureBox1"
        Me.PictureBox1.Size = New System.Drawing.Size(32, 32)
        Me.PictureBox1.TabIndex = 13
        Me.PictureBox1.TabStop = False
        '
        'ttRTPstat
        '
        Me.ttRTPstat.ToolTipTitle = "audio statistics:"
        '
        'Label2
        '
        Me.Label2.Location = New System.Drawing.Point(295, 159)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(50, 21)
        Me.Label2.TabIndex = 85
        Me.Label2.Text = "Transfer"
        Me.Label2.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'cbTransferList
        '
        Me.cbTransferList.FormattingEnabled = True
        Me.cbTransferList.Location = New System.Drawing.Point(350, 159)
        Me.cbTransferList.Name = "cbTransferList"
        Me.cbTransferList.Size = New System.Drawing.Size(198, 21)
        Me.cbTransferList.TabIndex = 86
        '
        'cbTargetList
        '
        Me.cbTargetList.FormattingEnabled = True
        Me.cbTargetList.Location = New System.Drawing.Point(350, 186)
        Me.cbTargetList.Name = "cbTargetList"
        Me.cbTargetList.Size = New System.Drawing.Size(198, 21)
        Me.cbTargetList.TabIndex = 88
        '
        'Label3
        '
        Me.Label3.Location = New System.Drawing.Point(295, 186)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(50, 21)
        Me.Label3.TabIndex = 87
        Me.Label3.Text = "To"
        Me.Label3.TextAlign = System.Drawing.ContentAlignment.MiddleCenter
        '
        'Button1
        '
        Me.Button1.Location = New System.Drawing.Point(554, 159)
        Me.Button1.Name = "Button1"
        Me.Button1.Size = New System.Drawing.Size(42, 48)
        Me.Button1.TabIndex = 89
        Me.Button1.Text = "Go"
        '
        'pbVideoPreview
        '
        Me.pbVideoPreview.Location = New System.Drawing.Point(350, 217)
        Me.pbVideoPreview.Name = "pbVideoPreview"
        Me.pbVideoPreview.Size = New System.Drawing.Size(176, 144)
        Me.pbVideoPreview.TabIndex = 90
        Me.pbVideoPreview.TabStop = False
        '
        'cbWebcam
        '
        Me.cbWebcam.Enabled = False
        Me.cbWebcam.FormattingEnabled = True
        Me.cbWebcam.Location = New System.Drawing.Point(16, 158)
        Me.cbWebcam.Name = "cbWebcam"
        Me.cbWebcam.Size = New System.Drawing.Size(140, 21)
        Me.cbWebcam.TabIndex = 91
        Me.cbWebcam.Text = "Default Webcam"
        '
        'Form1
        '
        Me.AutoScaleBaseSize = New System.Drawing.Size(5, 13)
        Me.ClientSize = New System.Drawing.Size(982, 368)
        Me.Controls.Add(Me.cbWebcam)
        Me.Controls.Add(Me.pbVideoPreview)
        Me.Controls.Add(Me.Button1)
        Me.Controls.Add(Me.cbTargetList)
        Me.Controls.Add(Me.Label3)
        Me.Controls.Add(Me.cbTransferList)
        Me.Controls.Add(Me.Label2)
        Me.Controls.Add(Me.bC4_3)
        Me.Controls.Add(Me.bC4_2)
        Me.Controls.Add(Me.bC3_3)
        Me.Controls.Add(Me.bC3_2)
        Me.Controls.Add(Me.bC2_3)
        Me.Controls.Add(Me.bC2_2)
        Me.Controls.Add(Me.bC1_3)
        Me.Controls.Add(Me.bC1_2)
        Me.Controls.Add(Me.bC4_1)
        Me.Controls.Add(Me.lC_callinfo4)
        Me.Controls.Add(Me.bC3_1)
        Me.Controls.Add(Me.lC_callinfo3)
        Me.Controls.Add(Me.bC2_1)
        Me.Controls.Add(Me.lC_callinfo2)
        Me.Controls.Add(Me.bC1_1)
        Me.Controls.Add(Me.lC_callinfo1)
        Me.Controls.Add(Me.Label6)
        Me.Controls.Add(Me.Label5)
        Me.Controls.Add(Me.cbUploadB)
        Me.Controls.Add(Me.cbDownloadB)
        Me.Controls.Add(Me.cbRecordDevice)
        Me.Controls.Add(Me.cbPlayDevice)
        Me.Controls.Add(Me.pbVideo)
        Me.Controls.Add(Me.cIce)
        Me.Controls.Add(Me.bExit)
        Me.Controls.Add(Me.cAec)
        Me.Controls.Add(Me.lCallStatus)
        Me.Controls.Add(Me.lOnlineStatus)
        Me.Controls.Add(Me.PictureBox1)
        Me.Controls.Add(Me.tCallee)
        Me.Controls.Add(Me.lCallee)
        Me.Controls.Add(Me.tProxy)
        Me.Controls.Add(Me.lProxy)
        Me.Controls.Add(Me.Label1)
        Me.Controls.Add(Me.lHelloWorld)
        Me.Controls.Add(Me.tPassword)
        Me.Controls.Add(Me.lPassword)
        Me.Controls.Add(Me.tUsername)
        Me.Controls.Add(Me.lUsername)
        Me.Controls.Add(Me.bRegister)
        Me.Controls.Add(Me.bInitAmsip)
        Me.Controls.Add(Me.bStartCall)
        Me.Icon = CType(resources.GetObject("$this.Icon"), System.Drawing.Icon)
        Me.Name = "Form1"
        Me.Text = "Antisip - Test App"
        CType(Me.bC4_3, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC4_2, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC3_3, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC3_2, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC2_3, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC2_2, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC1_3, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC1_2, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC4_1, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC3_1, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC2_1, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.bC1_1, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.pbVideo, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).EndInit()
        CType(Me.pbVideoPreview, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub

#End Region

    Private Sub bStartCall_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bStartCall.Click

        Dim Ret As Integer
        Ret = myapi.API_am_session_start(tCallee.Text)

        Me.PrintCallInfo()
        If (Ret >= 0) Then

        End If

    End Sub

    Private Sub bInitAmsip_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bInitAmsip.Click

        Dim Ret As Integer

        If (amsipRunning = True) Then
            amsipRunning = False
            TimerAmsip.Stop()
            TimerAmsip.Enabled = False
        End If

        bInitAmsip.Enabled = False
        bStartCall.Enabled = True
        bRegister.Enabled = False

        amsipProxy = tProxy.Text
        amsipUsername = tUsername.Text
        amsipPassword = tPassword.Text

        Ret = myapi.API_am_config(amsipUsername, amsipPassword, amsipProxy)

        myapi.local_config.AEC_on = cAec.Checked
        If (cIce.Checked = False) Then
            myapi.local_config.stun_server = Nothing
        End If

        Ret = myapi.API_save_am_config("amsip_cfg.xml")

        Ret = myapi.API_am_init(pbVideo.Handle, pbVideoPreview.Handle)
        bInitAmsip.Enabled = True

        If (Ret >= 0) Then
            bRegister.Enabled = True

            lOnlineStatus.Text = myapi.rid_status
            If (myapi.rid = 0) Then
                lOnlineStatus.BackColor = System.Drawing.Color.White
            ElseIf (myapi.rid <= 0) Then
                lOnlineStatus.BackColor = System.Drawing.Color.RosyBrown
            ElseIf (myapi.rid > 0) Then
                If (myapi.rid_code >= 200 And myapi.rid_code <= 299) Then
                    lOnlineStatus.BackColor = System.Drawing.Color.GreenYellow
                ElseIf (myapi.rid_code = 401 Or myapi.rid_code = 407) Then
                    lOnlineStatus.BackColor = System.Drawing.Color.Red
                Else
                    lOnlineStatus.BackColor = System.Drawing.Color.Pink
                End If
            End If

            Dim card_found As Boolean
            Dim sndcard As amsipapi.AM_SNDCARD
            sndcard = New amsipapi.AM_SNDCARD

            Me.cbPlayDevice.Items.Clear()
            Me.cbRecordDevice.Items.Clear()

            sndcard.card = 0
            card_found = False
            Do
                Ret = myapi.API_am_option_find_out_sound_card(sndcard)
                If (Ret >= 0) Then
                    Me.cbPlayDevice.Items.AddRange(New Object() {sndcard.name})
                    If (card_found = False) Then
                        card_found = True
                        Me.cbPlayDevice.Text = sndcard.name
                    End If
                End If
                sndcard.card = sndcard.card + 1
            Loop Until (Ret < 0)

            sndcard.card = 0
            card_found = False
            Do
                Ret = myapi.API_am_option_find_in_sound_card(sndcard)
                If (Ret >= 0) Then
                    Me.cbRecordDevice.Items.AddRange(New Object() {sndcard.name})
                    If (card_found = False) Then
                        card_found = True
                        Me.cbRecordDevice.Text = sndcard.name
                    End If
                End If
                sndcard.card = sndcard.card + 1
            Loop Until (Ret < 0)

            cbPlayDevice.Enabled = True
            cbRecordDevice.Enabled = True

            Dim camera As amsipapi.AM_CAMERA
            camera = New amsipapi.AM_CAMERA
            Me.cbWebcam.Items.Clear()

            camera.card = 0
            card_found = False
            Do
                Ret = myapi.API_am_option_find_camera(camera)
                If (Ret >= 0) Then
                    Me.cbWebcam.Items.AddRange(New Object() {camera.name})
                    If (card_found = False) Then
                        card_found = True
                        Me.cbWebcam.Text = camera.name
                    End If
                End If
                camera.card = camera.card + 1
            Loop Until (Ret < 0)

            cbWebcam.Enabled = True

            Dim upload_b As Integer
            Dim download_b As Integer
            upload_b = 128
            download_b = 256
            If ("128k" = cbUploadB.SelectedItem) Then
                upload_b = 64
            ElseIf ("256k" = cbUploadB.SelectedItem) Then
                upload_b = 128
            ElseIf ("512k" = cbUploadB.SelectedItem) Then
                upload_b = 256
            ElseIf ("740k" = cbUploadB.SelectedItem) Then
                upload_b = 512
            ElseIf (">512k" = cbUploadB.SelectedItem) Then
                upload_b = 1024
            End If

            If ("128k" = cbDownloadB.SelectedItem) Then
                download_b = 64
            ElseIf ("256k" = cbDownloadB.SelectedItem) Then
                download_b = 128
            ElseIf ("512k" = cbDownloadB.SelectedItem) Then
                download_b = 256
            ElseIf ("740k" = cbDownloadB.SelectedItem) Then
                download_b = 512
            ElseIf (">512k" = cbDownloadB.SelectedItem) Then
                download_b = 1024
            End If
            myapi.API_am_video_codec_attr_modify(upload_b, download_b)

            amsipRunning = True
            amsipTimerRunning = True
            TimerAmsip.Start()
            TimerAmsip.Enabled = True
        End If

    End Sub

    Private Sub bRegister_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bRegister.Click

        Dim Ret As Integer
        Ret = myapi.API_am_register()

        If (Ret >= 0) Then
            bStartCall.Enabled = True
            bRegister.Enabled = False
        End If

        lOnlineStatus.Text = myapi.rid_status
        If (myapi.rid = 0) Then
            lOnlineStatus.BackColor = System.Drawing.Color.White
        ElseIf (myapi.rid <= 0) Then
            lOnlineStatus.BackColor = System.Drawing.Color.RosyBrown
        ElseIf (myapi.rid > 0) Then
            If (myapi.rid_code >= 200 And myapi.rid_code <= 299) Then
                lOnlineStatus.BackColor = System.Drawing.Color.GreenYellow
            ElseIf (myapi.rid_code = 401 Or myapi.rid_code = 407) Then
                lOnlineStatus.BackColor = System.Drawing.Color.Red
            Else
                lOnlineStatus.BackColor = System.Drawing.Color.Pink
            End If
        End If

    End Sub

    Public Sub PrintCallInfo()
        'lbCallList.Items.Clear()

        Dim phone_call As AM_CALL
        Dim cid As Integer
        Dim idx As Integer

        For idx = 1 To 4 Step 1

            Dim lC_callinfo As System.Windows.Forms.Label
            Dim b_1 As System.Windows.Forms.PictureBox
            Dim b_2 As System.Windows.Forms.PictureBox
            Dim b_3 As System.Windows.Forms.PictureBox

            If (idx = 1) Then
                lC_callinfo = Me.lC_callinfo1
                b_1 = Me.bC1_1
                b_2 = Me.bC1_2
                b_3 = Me.bC1_3
            ElseIf (idx = 2) Then
                lC_callinfo = Me.lC_callinfo2
                b_1 = Me.bC2_1
                b_2 = Me.bC2_2
                b_3 = Me.bC2_3
            ElseIf (idx = 3) Then
                lC_callinfo = Me.lC_callinfo3
                b_1 = Me.bC3_1
                b_2 = Me.bC3_2
                b_3 = Me.bC3_3
            Else
                lC_callinfo = Me.lC_callinfo4
                b_1 = Me.bC4_1
                b_2 = Me.bC4_2
                b_3 = Me.bC4_3
            End If

            cid = myapi.GetCallIndex(idx)
            If (cid > 0) Then
                phone_call = myapi.pending_calls(cid)

                If (myapi.GetActiveDialog(cid) > 0) Then
                    lC_callinfo.Text = myapi.pending_dialogs(myapi.GetActiveDialog(cid)).ToString()
                    lC_callinfo.BackColor = System.Drawing.Color.GreenYellow
                    b_1.Image = busy_ico
                    b_2.Image = hold_ico
                    b_3.Image = video_ico
                ElseIf (myapi.GetPendingDialog(cid) > 0) Then
                    lC_callinfo.Text = myapi.pending_dialogs(myapi.GetPendingDialog(cid)).ToString()
                    lC_callinfo.BackColor = System.Drawing.Color.RosyBrown
                    b_1.Image = busy_ico
                    b_2.Image = hold_ico
                    b_3.Image = video_ico
                ElseIf (myapi.GetTerminatedDialog(cid) > 0) Then
                    lC_callinfo.Text = myapi.pending_dialogs(myapi.GetTerminatedDialog(cid)).ToString()
                    lC_callinfo.BackColor = System.Drawing.Color.RosyBrown
                    b_1.Image = busy_ico
                    b_2.Image = Nothing
                    b_3.Image = Nothing
                Else
                    lC_callinfo.Text = phone_call.ToString()
                    lC_callinfo.BackColor = System.Drawing.Color.Red
                    b_1.Image = wfa_ico
                    b_2.Image = Nothing
                    b_3.Image = Nothing
                End If
            Else
                lC_callinfo.Text = "Status: Not in call"
                lC_callinfo.BackColor = System.Drawing.Color.White
                b_1.Image = online_ico
                b_2.Image = Nothing
                b_3.Image = Nothing
            End If
        Next


    End Sub

    Private Sub TimerAmsip_Tick(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles TimerAmsip.Tick
        TimerAmsip.Enabled = False
        TimerAmsip.Stop()

        Dim Ret As Integer
        Ret = myapi.API_process_events()

        If (Ret >= 0) Then

            lOnlineStatus.Text = myapi.rid_status
            If (myapi.rid = 0) Then
                lOnlineStatus.BackColor = System.Drawing.Color.White
            ElseIf (myapi.rid <= 0) Then
                lOnlineStatus.BackColor = System.Drawing.Color.RosyBrown
            ElseIf (myapi.rid > 0) Then
                If (myapi.rid_code >= 200 And myapi.rid_code <= 299) Then
                    lOnlineStatus.BackColor = System.Drawing.Color.GreenYellow
                ElseIf (myapi.rid_code = 401 Or myapi.rid_code = 407) Then
                    lOnlineStatus.BackColor = System.Drawing.Color.Red
                Else
                    lOnlineStatus.BackColor = System.Drawing.Color.Pink
                End If
            End If

        End If

        If (Ret >= 0) Then
            PrintCallInfo()
        End If

        If (myapi.rid_status = "Registration Deleted on server!") Then
            myapi.API_am_quit()
            Application.Exit()
            Return
        End If

        If (amsipRunning = True) Then
            TimerAmsip.Start()
            TimerAmsip.Enabled = True
        Else
            amsipTimerRunning = False
        End If

    End Sub

    Private Sub cAec_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cAec.CheckedChanged
        myapi.API_am_option_enable_echo_canceller(cAec.Checked)
    End Sub

    Private Sub bExit_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bExit.Click

        Dim Ret As Integer

        'If (amsipRunning = True) Then
        'amsipRunning = False
        'TimerAmsip.Stop()
        'TimerAmsip.Enabled = False
        'End If

        If (appClosing = True) Then
            myapi.API_am_quit()
            Application.Exit()
        End If

        If (amsipRunning = False) Then
            Application.Exit()
        End If

        appClosing = True
        Ret = myapi.API_am_unregister()

        'myapi.API_am_quit()
        'Application.Exit()
    End Sub

    Private Sub cbPlayDevice_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cbPlayDevice.SelectedIndexChanged
        Dim Ret As Integer
        Dim sndcard As amsipapi.AM_SNDCARD
        sndcard = New amsipapi.AM_SNDCARD
        sndcard.card = 0

        Do
            Ret = myapi.API_am_option_find_out_sound_card(sndcard)
            If (Ret >= 0) Then
                If (sndcard.name = cbPlayDevice.SelectedItem) Then
                    myapi.API_am_option_select_out_sound_card(sndcard.card)
                    Return
                End If
            End If
            sndcard.card = sndcard.card + 1
        Loop Until (Ret < 0)
    End Sub

    Private Sub cbRecordDevice_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cbRecordDevice.SelectedIndexChanged
        Dim Ret As Integer
        Dim sndcard As amsipapi.AM_SNDCARD
        sndcard = New amsipapi.AM_SNDCARD
        sndcard.card = 0

        Do
            Ret = myapi.API_am_option_find_in_sound_card(sndcard)
            If (Ret >= 0) Then
                If (sndcard.name = cbRecordDevice.SelectedItem) Then
                    myapi.API_am_option_select_in_sound_card(sndcard.card)
                    Return
                End If
            End If
            sndcard.card = sndcard.card + 1
        Loop Until (Ret < 0)
    End Sub


    Private Sub cbWebcam_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cbWebcam.SelectedIndexChanged
        Dim Ret As Integer
        Dim camera As amsipapi.AM_CAMERA
        camera = New amsipapi.AM_CAMERA
        camera.card = 0

        Do
            Ret = myapi.API_am_option_find_camera(camera)
            If (Ret >= 0) Then
                If (camera.name = cbWebcam.SelectedItem) Then
                    myapi.API_am_option_enable_preview(False)
                    myapi.API_am_option_select_camera(camera.card)
                    myapi.API_am_option_enable_preview(True)
                    Return
                End If
            End If
            camera.card = camera.card + 1
        Loop Until (Ret < 0)
    End Sub

    Private Sub cbDownloadB_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cbDownloadB.SelectedIndexChanged
        Dim upload_b As Integer
        Dim download_b As Integer
        upload_b = 128
        download_b = 256
        If ("128k" = cbUploadB.SelectedItem) Then
            upload_b = 64
        ElseIf ("256k" = cbUploadB.SelectedItem) Then
            upload_b = 128
        ElseIf ("512k" = cbUploadB.SelectedItem) Then
            upload_b = 256
        ElseIf ("740k" = cbUploadB.SelectedItem) Then
            upload_b = 512
        ElseIf (">512k" = cbUploadB.SelectedItem) Then
            upload_b = 1024
        End If

        If ("128k" = cbDownloadB.SelectedItem) Then
            download_b = 64
        ElseIf ("256k" = cbDownloadB.SelectedItem) Then
            download_b = 128
        ElseIf ("512k" = cbDownloadB.SelectedItem) Then
            download_b = 256
        ElseIf ("740k" = cbDownloadB.SelectedItem) Then
            download_b = 512
        ElseIf (">512k" = cbDownloadB.SelectedItem) Then
            download_b = 1024
        End If
        myapi.API_am_video_codec_attr_modify(upload_b, download_b)
    End Sub

    Private Sub cbUploadB_SelectedIndexChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cbUploadB.SelectedIndexChanged
        Dim upload_b As Integer
        Dim download_b As Integer
        upload_b = 128
        download_b = 256
        If ("128k" = cbUploadB.SelectedItem) Then
            upload_b = 64
        ElseIf ("256k" = cbUploadB.SelectedItem) Then
            upload_b = 128
        ElseIf ("512k" = cbUploadB.SelectedItem) Then
            upload_b = 256
        ElseIf ("740k" = cbUploadB.SelectedItem) Then
            upload_b = 512
        ElseIf (">512k" = cbUploadB.SelectedItem) Then
            upload_b = 1024
        End If

        If ("128k" = cbDownloadB.SelectedItem) Then
            download_b = 64
        ElseIf ("256k" = cbDownloadB.SelectedItem) Then
            download_b = 128
        ElseIf ("512k" = cbDownloadB.SelectedItem) Then
            download_b = 256
        ElseIf ("740k" = cbDownloadB.SelectedItem) Then
            download_b = 512
        ElseIf (">512k" = cbDownloadB.SelectedItem) Then
            download_b = 1024
        End If
        myapi.API_am_video_codec_attr_modify(upload_b, download_b)
    End Sub

    Private Function ClickStopCall(ByVal idx As Integer) As Integer

        Dim cid As Integer
        Dim did As Integer
        cid = myapi.GetCallIndex(idx)
        If (cid <= 0) Then
            Return -1
        End If

        did = 0
        If (myapi.GetActiveDialog(cid) > 0) Then
            did = myapi.GetActiveDialog(cid)
        ElseIf (myapi.GetPendingDialog(cid) > 0) Then
            did = myapi.GetPendingDialog(cid)
        End If

        If (myapi.GetCallIndex(idx) > 0) Then
            myapi.API_am_session_stop(cid, did, 486)
            Me.PrintCallInfo()
            Return 0
        End If

        Return -1
    End Function

    Private Function ClickHoldCall(ByVal idx As Integer) As Integer
        Dim cid As Integer
        Dim did As Integer
        cid = myapi.GetCallIndex(idx)
        If (cid <= 0) Then
            Return -1
        End If

        did = 0
        If (myapi.GetActiveDialog(cid) > 0) Then
            did = myapi.GetActiveDialog(cid)
        End If

        If (did <= 0) Then
            If (myapi.GetPendingDialog(cid) > 0) Then
                did = myapi.GetPendingDialog(cid)
                myapi.API_am_session_answer(did)
                Return 0
            End If
            Return -1
        End If

        If (myapi.GetCallIndex(idx) > 0) Then
            myapi.API_am_session_hold(did)
            Me.PrintCallInfo()
            Return 0
        End If

        Return -1
    End Function

    Private Function ClickAddVideo(ByVal idx As Integer) As Integer
        Dim cid As Integer
        Dim did As Integer
        cid = myapi.GetCallIndex(idx)
        If (cid <= 0) Then
            Return -1
        End If

        did = 0
        If (myapi.GetActiveDialog(cid) > 0) Then
            did = myapi.GetActiveDialog(cid)
        End If

        If (did < 0) Then
            Return -1
        End If

        If (myapi.GetCallIndex(idx) > 0) Then
            myapi.API_am_session_add_video(did)
            Me.PrintCallInfo()
            Return 0
        End If

        Return -1
    End Function


    Private Sub bC1_1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC1_1.Click
        ClickStopCall(1)
    End Sub

    Private Sub bC2_1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC2_1.Click
        ClickStopCall(2)
    End Sub

    Private Sub bC3_1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC3_1.Click
        ClickStopCall(3)
    End Sub

    Private Sub bC4_1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC4_1.Click
        ClickStopCall(4)
    End Sub


    Private Sub bC1_2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC1_2.Click
        ClickHoldCall(1)
    End Sub

    Private Sub bC2_2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC2_2.Click
        ClickHoldCall(2)
    End Sub

    Private Sub bC3_2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC3_2.Click
        ClickHoldCall(3)
    End Sub

    Private Sub bC4_2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC4_2.Click
        ClickHoldCall(4)
    End Sub

    Private Sub bC1_3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC1_3.Click
        ClickAddVideo(1)
    End Sub

    Private Sub bC2_3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC2_3.Click
        ClickAddVideo(2)
    End Sub

    Private Sub bC3_3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC3_3.Click
        ClickAddVideo(3)
    End Sub

    Private Sub bC4_3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles bC4_3.Click
        ClickAddVideo(4)
    End Sub

    Private Function ClickGetStatistics(ByVal idx As Integer, ByRef lCall As Label) As Integer

        Dim Ret As Integer

        Dim cid As Integer
        Dim did As Integer
        cid = myapi.GetCallIndex(idx)
        If (cid <= 0) Then
            Me.ttRTPstat.SetToolTip(lCall, "No call pending")
            Return -1
        End If

        did = 0
        If (myapi.GetActiveDialog(cid) > 0) Then
            did = myapi.GetActiveDialog(cid)
        End If

        If (did < 0) Then
            Me.ttRTPstat.SetToolTip(lCall, "No active call")
            Return -1
        End If

        Dim audio_stats As amsipapi.AM_AUDIO_STATS
        Ret = myapi.API_am_session_get_audio_statistics(did, audio_stats)
        If (Ret >= 0 And audio_stats.outgoing_sent > 0) Then
            Dim rtp_stats As String

            rtp_stats = "RTP -> IN: " + audio_stats.incoming_received.ToString() + Environment.NewLine
            rtp_stats += "RTP -> OUT: " + audio_stats.outgoing_sent.ToString() + Environment.NewLine
            rtp_stats += "RTP -> LOSS: " + audio_stats.incoming_packetloss.ToString() + " (" + audio_stats.pk_loss.ToString + ")" + Environment.NewLine
            rtp_stats += "RTP -> LATE: " + audio_stats.incoming_outoftime.ToString() + Environment.NewLine

            rtp_stats += "MIX -> PROCESSED: " + audio_stats.msconf_processed.ToString() + Environment.NewLine
            rtp_stats += "MIX -> MISSED: " + audio_stats.msconf_missed.ToString() + Environment.NewLine
            rtp_stats += "MIX -> DISCARDED: " + audio_stats.msconf_discarded.ToString() + Environment.NewLine

            rtp_stats += "SND -> RECORDED: " + audio_stats.sndcard_recorded.ToString() + Environment.NewLine
            rtp_stats += "SND -> PLAYED: " + audio_stats.sndcard_played.ToString() + Environment.NewLine
            rtp_stats += "SND -> DISCARDED: " + audio_stats.sndcard_discarded.ToString()

            'Get audio/rtp statistics
            Me.ttRTPstat.SetToolTip(lCall, rtp_stats)
            Return 0
        End If

        Me.ttRTPstat.SetToolTip(lCall, "No stat available for this call")
        Return -1

    End Function

    Private Sub lC_callinfo1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles lC_callinfo1.Click
        ClickGetStatistics(1, lC_callinfo1)
    End Sub

    Private Sub lC_callinfo2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles lC_callinfo2.Click
        ClickGetStatistics(2, lC_callinfo2)
    End Sub

    Private Sub lC_callinfo3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles lC_callinfo3.Click
        ClickGetStatistics(3, lC_callinfo3)
    End Sub

    Private Sub lC_callinfo4_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles lC_callinfo4.Click
        ClickGetStatistics(4, lC_callinfo4)
    End Sub

    Private Sub cbTransferList_DropDown(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cbTransferList.DropDown

        Dim idx As Integer
        Me.cbTransferList.Items.Clear()

        idx = 1
        Do
            Dim cid As Integer
            Dim did As Integer
            cid = myapi.GetCallIndex(idx)
            If (cid > 0) Then
                did = 0
                If (myapi.GetActiveDialog(cid) > 0) Then
                    did = myapi.GetActiveDialog(cid)
                End If
                If (did > 0) Then
                    Dim phone_dialog As AM_DIALOG
                    phone_dialog = myapi.pending_dialogs(did)
                    Me.cbTransferList.Items.AddRange(New Object() {phone_dialog.remote_address.ToString()})
                End If
            End If


            idx = idx + 1
        Loop Until (idx >= 4)

    End Sub

    Private Sub cbTargetList_DropDown(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles cbTargetList.DropDown
        Dim idx As Integer
        Me.cbTargetList.Items.Clear()

        idx = 1
        Do
            Dim cid As Integer
            Dim did As Integer
            cid = myapi.GetCallIndex(idx)
            If (cid > 0) Then
                did = 0
                If (myapi.GetActiveDialog(cid) > 0) Then
                    did = myapi.GetActiveDialog(cid)
                End If
                If (did > 0) Then
                    Dim phone_dialog As AM_DIALOG
                    phone_dialog = myapi.pending_dialogs(did)
                    Me.cbTargetList.Items.AddRange(New Object() {phone_dialog.remote_address.ToString()})
                End If
            End If


            idx = idx + 1
        Loop Until (idx >= 4)

    End Sub

    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click

        ' Search the selected "transfer" and "target" dialogs
        Dim idx As Integer

        Dim did_transfer As Integer = 0
        Dim did_target As Integer = 0
        Dim cid_target As Integer = 0

        If (cbTransferList.Text = Nothing) Then
            Return
        End If
        If (cbTargetList.Text = Nothing) Then
            Return
        End If

        idx = 1
        Do
            Dim cid As Integer
            Dim did As Integer
            cid = myapi.GetCallIndex(idx)
            If (cid > 0) Then
                did = 0
                If (myapi.GetActiveDialog(cid) > 0) Then
                    did = myapi.GetActiveDialog(cid)
                End If
                If (did > 0) Then
                    Dim phone_dialog As AM_DIALOG
                    phone_dialog = myapi.pending_dialogs(did)
                    If (String.Compare(cbTransferList.Text, phone_dialog.remote_address) = 0) Then
                        'Found call to transfer
                        did_transfer = did
                    End If
                End If
            End If

            idx = idx + 1
        Loop Until (idx >= 4)

        If (did_transfer = 0) Then
            'initiate transfer OUTSIDE call!

            Return
        End If

        idx = 1
        Do
            Dim did As Integer
            cid_target = myapi.GetCallIndex(idx)
            If (cid_target > 0) Then
                did = 0
                If (myapi.GetActiveDialog(cid_target) > 0) Then
                    did = myapi.GetActiveDialog(cid_target)
                End If
                If (did > 0) Then
                    Dim phone_dialog As AM_DIALOG
                    phone_dialog = myapi.pending_dialogs(did)
                    If (String.Compare(cbTargetList.Text, phone_dialog.remote_address) = 0) Then
                        'Found call to transfer
                        did_target = did
                    End If
                End If
            End If

            idx = idx + 1
        Loop Until (idx >= 4)


        Dim phone_dialog1 As AM_DIALOG
        phone_dialog1 = myapi.pending_dialogs(did_transfer)

        If (did_target = 0) Then
            'initiate blind transfer!
            phone_dialog1.refer_cid = 0
            phone_dialog1.refer_did = 0
            myapi.API_am_session_refer(did_transfer, cbTargetList.Text)
            Return
        End If

        'initiate attented transfer!

        Dim uri As String
        uri = myapi.API_am_session_get_referto(did_target)
        If (uri = Nothing) Then
            Return
        End If

        phone_dialog1.refer_cid = cid_target
        phone_dialog1.refer_did = did_target
        myapi.API_am_session_refer(did_transfer, uri)
        Return

    End Sub

End Class

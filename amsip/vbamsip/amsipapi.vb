Imports System.IO
Imports System.Xml.Serialization


Public Class AM_CONFIG
    Public username As String = "test1"
    Public password As String = "secret"
    Public domain_realm As String = "sip.antisip.com"

    'Generated sip config:
    Public proxy As String = "sip:sip.antisip.com"
    Public identity As String = "sip:test1@sip.antisip.com"

    'various options:
    Public stun_server As String = "stun.goober.com"
    Public sip_port As Short = 6010
    Public protocol As String = "UDP"

    'compliance with other bad configured SIP networks:
    Public ip_of_proxy As String = Nothing
    Public authentication_realm As String = Nothing

    'audio options:
    Public in_card As String = Nothing
    Public out_card As String = Nothing
    Public ring_card As String = Nothing
    Public AEC_on As Boolean = False
    Public AGC_on As Boolean = False

    Public hold_file As String = "holdmusic.wav"

    Public debug_level As Short = 6

    Public Function Save(ByVal cfg_file As String) As Integer

        If (username = Nothing Or password = Nothing Or domain_realm = Nothing) Then
            Return -1
        End If

        'Serialize object to a text file.
        Dim objStreamWriter As New StreamWriter(cfg_file)
        Dim x As New XmlSerializer(Me.GetType)
        x.Serialize(objStreamWriter, Me)
        objStreamWriter.Close()
        Return 0
    End Function

    Public Function Wizard(ByVal file As String) As Integer

        If (username = Nothing Or password = Nothing Or domain_realm = Nothing) Then
            Return -1
        End If

        Return 0
    End Function

End Class


Public Class AM_CALL
    Public cid As Integer
    Public tid As Integer
    Public did As Integer
    Public remote_address As String

    Public state As Integer '0: noanswer 1:earlydialog 2:confirmeddialog
    Public cid_code As Integer
    Public cid_status As String

    Public call_idx As Integer

    Public Overrides Function ToString() As String
        Dim txt As String

        txt = "no call pending..."
        If (state < 1) Then
            txt = "---? " + remote_address + " // " + " Waiting for answer from remote party"
        ElseIf (state < 2) Then
            txt = "---> " + remote_address + " // " + " Remote party is being contacted"
        ElseIf (state < 3) Then
            txt = "<--> " + remote_address + " // " + " Call established"
        ElseIf (state < 4) Then
            txt = "---- " + remote_address + " // " + " Call terminated"
        End If

        Return txt
    End Function

End Class

Public Class AM_DIALOG
    Public cid As Integer
    Public tid As Integer
    Public did As Integer
    Public remote_address As String
    Public state As Integer '0: pending, 1: established, 2: stopped
    Public cid_code As Integer
    Public cid_status As String
    Public audio_sendrecv As Integer = 0
    Public muted As Integer = 0
    Public video_sendrecv As Integer = -1

    Public refer_cid As Integer = -1
    Public refer_did As Integer = -1

    Public Overrides Function ToString() As String
        Dim txt As String

        txt = "no dialog pending..."
        If (state < 1) Then
            txt = "---> " + remote_address + " // " + cid_code.ToString() + cid_status
        ElseIf (state < 2) Then
            txt = "<---> " + remote_address + " // " + cid_code.ToString() + cid_status
        ElseIf (state < 3) Then
            txt = "<--> " + remote_address + " // " + cid_code.ToString() + cid_status + " Call established"
        ElseIf (state < 4) Then
            txt = "---- " + remote_address + " // " + " Call terminated"
        End If

        Return txt
    End Function

End Class


Public Class amsipapi

    Public Structure AM_VIDEO_CODEC_INFO
        Dim enable As Integer
        <VBFixedString(64), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=64)> Public name As String
        Dim payload As Integer
        Dim freq As Integer
        Dim mode As Integer
        Dim vbr As Integer
        Dim cng As Integer
        Dim vad As Integer
        Dim dtx As Integer
    End Structure

    Public Structure AM_VIDEO_CODEC_ATTR
        Dim ptime As Integer
        Dim maxptime As Integer
        Dim upload_bandwidth As Integer
        Dim download_bandwidth As Integer
    End Structure

    Public Structure AM_SNDCARD
        Dim card As Integer
        <VBFixedString(256), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=256)> Public name As String
        Dim capabilities As Integer
        <VBFixedString(256), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=256)> Public driver_type As String
    End Structure

    Public Structure AM_CAMERA
        Dim card As Integer
        <VBFixedString(256), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=256)> Public name As String
    End Structure

    Public Structure EXOSIP_EVENT
        Dim type As Short
        <VBFixedString(256), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=256)> Public textinfo As String
        Dim external_reference As System.IntPtr
        Dim request As System.IntPtr
        Dim response As System.IntPtr
        Dim ack As System.IntPtr
        Dim tid As Integer
        Dim did As Integer
        Dim rid As Integer
        Dim cid As Integer
        Dim sid As Integer
        Dim nid As Integer
        Dim ss_status As Integer
        Dim ss_reason As Integer
    End Structure

    Private Structure AM_MESSAGEINFO
        Dim answer_code As Integer
        <VBFixedString(128), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=128)> Public method As String
        <VBFixedString(1024), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=1024)> Public reason As String
        <VBFixedString(1024), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=1024)> Public target As String

        <VBFixedString(1024), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=1024)> Public sip_from As String
        <VBFixedString(1024), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=1024)> Public sip_to As String
        <VBFixedString(1024), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=1024)> Public sip_contact As String
        <VBFixedString(1024), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=1024)> Public sip_call_id As String
    End Structure

    Public Structure AM_AUDIO_STATS
        Dim proposed_action As Integer
        Dim pk_loss As Integer

        Dim incoming_received As Integer
        Dim incoming_expected As Integer
        Dim incoming_packetloss As Integer
        Dim incoming_outoftime As Integer
        Dim incoming_notplayed As Integer
        Dim incoming_discarded As Integer

        Dim outgoing_sent As Integer

        Dim sndcard_recorded As Integer
        Dim sndcard_played As Integer
        Dim sndcard_discarded As Integer
        Dim msconf_processed As Integer
        Dim msconf_missed As Integer
        Dim msconf_discarded As Integer
    End Structure

    Public Structure AM_HEADER
        Dim index As Integer
        <VBFixedString(1024), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=1024)> Public header_value As String
    End Structure

    Public Structure AM_BODYINFO
        Dim index As Integer
        Dim content_length As Integer
        <VBFixedString(1024), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=1024)> Public content_type As String
        '<System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.LPWStr)>
        Dim attachment As IntPtr
    End Structure

    '#Const DecoratedName = 1

#If DecoratedName Then

    Private Declare Function am_init Lib "amsip" Alias "_am_init@8" (ByVal a As String, ByVal c As Short) As Short
    Private Declare Function am_reset Lib "amsip" Alias "_am_reset@8" (ByVal a As String, ByVal c As Short) As Short
    Private Declare Function am_option_add_dns_cache Lib "amsip" Alias "_am_option_add_dns_cache@8" (ByVal a As String, ByVal b As String) As Short

    Private Declare Function am_quit Lib "amsip" Alias "_am_quit@0" () As Short
    Private Declare Function am_option_enable_turn_server Lib "amsip" Alias "_am_option_enable_turn_server@8" (ByVal a As String, ByVal c As Short) As Short
    Private Declare Function am_option_enable_stun_server Lib "amsip" Alias "_am_option_enable_stun_server@8" (ByVal a As String, ByVal c As Short) As Short

    Private Declare Function am_network_start Lib "amsip" Alias "_am_network_start@8" (ByVal a As String, ByVal b As Short) As Short
    Private Declare Function am_option_set_password Lib "amsip" Alias "_am_option_set_password@12" (ByVal a As String, ByVal b As String, ByVal c As String) As Short
    Private Declare Function am_option_enable_echo_canceller Lib "amsip" Alias "_am_option_enable_echo_canceller@12" (ByVal a As Short, ByVal b As Short, ByVal c As Short) As Short
    Private Declare Function am_option_enable_optionnal_encryption Lib "amsip" Alias "_am_option_enable_optionnal_encryption@4" (ByVal a As Short) As Short

    Private Declare Function am_register_start Lib "amsip" Alias "_am_register_start@16" (ByVal a As String, ByVal b As String, ByVal c As Short, ByVal d As Short) As Short
    Private Declare Function am_register_send_star Lib "amsip" Alias "_am_register_send_star@8" (ByVal a As String, ByVal b As String) As Short

    Private Declare Function am_session_start Lib "amsip" Alias "_am_session_start@16" (ByVal a As String, ByVal b As String, ByVal c As String, ByVal d As String) As Short
    Private Declare Function am_session_start_with_video Lib "amsip" Alias "_am_session_start_with_video@16" (ByVal a As String, ByVal b As String, ByVal c As String, ByVal d As String) As Short

    Private Declare Function am_session_answer Lib "amsip" Alias "_am_session_answer@16" (ByVal a As Short, ByVal b As Short, ByVal c As Short, ByVal d As Short) As Short
    Private Declare Function am_session_stop Lib "amsip" Alias "_am_session_stop@12" (ByVal a As Short, ByVal b As Short, ByVal c As Short) As Short
    Private Declare Function am_session_refer Lib "amsip" Alias "_am_session_refer@12" (ByVal a As Short, ByVal b As String, ByVal c As String) As Short
    Private Declare Function am_session_get_referto Lib "amsip" Alias "_am_session_get_referto@12" (ByVal a As Short, ByRef b As AM_STRING256, ByVal c As Short) As Short
    Private Declare Function am_session_find_by_replaces Lib "amsip" Alias "_am_session_find_by_replaces@4" (ByVal msg As System.IntPtr) As Short

    Private Declare Function am_session_hold Lib "amsip" Alias "_am_session_hold@8" (ByVal a As Short, ByVal b As String) As Short
    Private Declare Function am_session_off_hold Lib "amsip" Alias "_am_session_off_hold@4" (ByVal a As Short) As Short
    Private Declare Function am_session_mute Lib "amsip" Alias "_am_session_mute@4" (ByVal a As Short) As Short
    Private Declare Function am_session_unmute Lib "amsip" Alias "_am_session_unmute@4" (ByVal a As Short) As Short
    Private Declare Function am_session_add_video Lib "amsip" Alias "_am_session_add_video@4" (ByVal a As Short) As Short
    Private Declare Function am_session_get_audio_statistics Lib "amsip" Alias "_am_session_get_audio_statistics@8" (ByVal a As Short, ByRef audio_stats As AM_AUDIO_STATS) As Short
    Private Declare Function am_session_answer_request Lib "amsip" Alias "_am_session_answer_request@12" (ByVal a As Short, ByVal b As Short, ByVal c As Short) As Short
    Private Declare Function am_session_record Lib "amsip" Alias "_am_session_record@8" (ByVal a As Short, ByVal b As String) As Short

    Private Declare Function am_option_select_in_sound_card Lib "amsip" Alias "_am_option_select_in_sound_card@4" (ByVal a As Short) As Short
    Private Declare Function am_option_select_out_sound_card Lib "amsip" Alias "_am_option_select_out_sound_card@4" (ByVal a As Short) As Short
    Private Declare Function am_option_find_in_sound_card Lib "amsip" Alias "_am_option_find_in_sound_card@4" (ByRef a As amsipapi.AM_SNDCARD) As Short
    Private Declare Function am_option_find_out_sound_card Lib "amsip" Alias "_am_option_find_out_sound_card@4" (ByRef a As amsipapi.AM_SNDCARD) As Short
    Private Declare Function am_option_set_window_handle Lib "amsip" Alias "_am_option_set_window_handle@12" (ByVal a As IntPtr, ByVal b As Short, ByVal c As Short) As Short
    Private Declare Function am_option_set_window_preview_handle Lib "amsip" Alias "_am_option_set_window_preview_handle@12" (ByVal a As IntPtr, ByVal b As Short, ByVal c As Short) As Short

    Private Declare Function am_option_select_camera Lib "amsip" Alias "_am_option_select_camera@4" (ByVal a As Short) As Short
    Private Declare Function am_option_find_camera Lib "amsip" Alias "_am_option_find_camera@4" (ByRef a As amsipapi.AM_CAMERA) As Short

    Private Declare Function am_option_enable_preview Lib "amsip" Alias "_am_option_enable_preview@4" (ByVal c As Short) As Short
    Private Declare Function am_video_codec_attr_modify Lib "amsip" Alias "_am_video_codec_attr_modify@4" (ByRef a As amsipapi.AM_VIDEO_CODEC_ATTR) As Short
    Private Declare Function am_video_codec_info_modify Lib "amsip" Alias "_am_video_codec_info_modify@8" (ByRef a As amsipapi.AM_VIDEO_CODEC_INFO, ByVal b As Short) As Short

    Private Declare Function am_message_answer Lib "amsip" Alias "_am_message_answer@8" (ByVal a As Short, ByVal b As Short) As Short

    Private Declare Function am_event_get Lib "amsip" Alias "_am_event_get@4" (ByRef a As amsipapi.EXOSIP_EVENT) As Short
    Private Declare Function am_event_release Lib "amsip" Alias "_am_event_release@4" (ByRef a As amsipapi.EXOSIP_EVENT) As Short

    Private Declare Function am_message_get_header Lib "amsip" Alias "_am_message_get_header@12" (ByVal msg As System.IntPtr, ByVal headername As String, ByRef mheader As amsipapi.AM_HEADER) As Short
    Private Declare Function am_message_get_bodyinfo Lib "amsip" Alias "_am_message_get_bodyinfo@12" (ByVal msg As System.IntPtr, ByVal attachement_idx As Integer, ByRef mbodyinfo As amsipapi.AM_BODYINFO) As Short
    Private Declare Function am_message_release_bodyinfo Lib "amsip" Alias "_am_message_release_bodyinfo@4" (ByRef mbodyinfo As amsipapi.AM_BODYINFO) As Short

    Private Declare Function am_message_get_messageinfo Lib "amsip" Alias "_am_message_get_messageinfo@8" (ByVal msg As System.IntPtr, ByRef minfo As amsipapi.AM_MESSAGEINFO) As Short

#Else

    Private Declare Function am_init Lib "amsip" Alias "am_init" (ByVal a As String, ByVal c As Short) As Short
    Private Declare Function am_reset Lib "amsip" Alias "am_reset" (ByVal a As String, ByVal c As Short) As Short
    Private Declare Function am_option_add_dns_cache Lib "amsip" Alias "am_option_add_dns_cache" (ByVal a As String, ByVal b As String) As Short

    Private Declare Function am_quit Lib "amsip" Alias "am_quit" () As Short
    Private Declare Function am_option_enable_turn_server Lib "amsip" Alias "am_option_enable_turn_server" (ByVal a As String, ByVal c As Short) As Short
    Private Declare Function am_option_enable_stun_server Lib "amsip" Alias "am_option_enable_stun_server" (ByVal a As String, ByVal c As Short) As Short

    Private Declare Function am_network_start Lib "amsip" Alias "am_network_start" (ByVal a As String, ByVal b As Short) As Short
    Private Declare Function am_option_set_password Lib "amsip" Alias "am_option_set_password" (ByVal a As String, ByVal b As String, ByVal c As String) As Short
    Private Declare Function am_option_enable_echo_canceller Lib "amsip" Alias "am_option_enable_echo_canceller" (ByVal a As Short, ByVal b As Short, ByVal c As Short) As Short
    Private Declare Function am_option_enable_optionnal_encryption Lib "amsip" Alias "am_option_enable_optionnal_encryption" (ByVal a As Short) As Short

    Private Declare Function am_register_start Lib "amsip" Alias "am_register_start" (ByVal a As String, ByVal b As String, ByVal c As Short, ByVal d As Short) As Short
    Private Declare Function am_register_send_star Lib "amsip" Alias "am_register_send_star" (ByVal a As String, ByVal b As String) As Short

    Private Declare Function am_session_start Lib "amsip" Alias "am_session_start" (ByVal a As String, ByVal b As String, ByVal c As String, ByVal d As String) As Short
    Private Declare Function am_session_start_with_video Lib "amsip" Alias "am_session_start_with_video" (ByVal a As String, ByVal b As String, ByVal c As String, ByVal d As String) As Short

    Private Declare Function am_session_answer Lib "amsip" Alias "am_session_answer" (ByVal a As Short, ByVal b As Short, ByVal c As Short, ByVal d As Short) As Short
    Private Declare Function am_session_stop Lib "amsip" Alias "am_session_stop" (ByVal a As Short, ByVal b As Short, ByVal c As Short) As Short
    Private Declare Function am_session_refer Lib "amsip" Alias "am_session_refer" (ByVal a As Short, ByVal b As String, ByVal c As String) As Short
    Private Declare Function am_session_get_referto Lib "amsip" Alias "am_session_get_referto" (ByVal a As Short, ByRef b As AM_STRING256, ByVal c As Short) As Short
    Private Declare Function am_session_find_by_replaces Lib "amsip" Alias "am_session_find_by_replaces" (ByVal msg As System.IntPtr) As Short

    Private Declare Function am_session_hold Lib "amsip" Alias "am_session_hold" (ByVal a As Short, ByVal b As String) As Short
    Private Declare Function am_session_off_hold Lib "amsip" Alias "am_session_off_hold" (ByVal a As Short) As Short
    Private Declare Function am_session_mute Lib "amsip" Alias "am_session_mute" (ByVal a As Short) As Short
    Private Declare Function am_session_unmute Lib "amsip" Alias "am_session_unmute" (ByVal a As Short) As Short
    Private Declare Function am_session_add_video Lib "amsip" Alias "am_session_add_video" (ByVal a As Short) As Short
    Private Declare Function am_session_get_audio_statistics Lib "amsip" Alias "am_session_get_audio_statistics" (ByVal a As Short, ByRef audio_stats As AM_AUDIO_STATS) As Short
    Private Declare Function am_session_answer_request Lib "amsip" Alias "am_session_answer_request" (ByVal a As Short, ByVal b As Short, ByVal c As Short) As Short
    Private Declare Function am_session_record Lib "amsip" Alias "am_session_record" (ByVal a As Short, ByVal b As String) As Short

    Private Declare Function am_option_select_in_sound_card Lib "amsip" Alias "am_option_select_in_sound_card" (ByVal a As Short) As Short
    Private Declare Function am_option_select_out_sound_card Lib "amsip" Alias "am_option_select_out_sound_card" (ByVal a As Short) As Short
    Private Declare Function am_option_find_in_sound_card Lib "amsip" Alias "am_option_find_in_sound_card" (ByRef a As amsipapi.AM_SNDCARD) As Short
    Private Declare Function am_option_find_out_sound_card Lib "amsip" Alias "am_option_find_out_sound_card" (ByRef a As amsipapi.AM_SNDCARD) As Short
    Private Declare Function am_option_set_window_handle Lib "amsip" Alias "am_option_set_window_handle" (ByVal a As IntPtr, ByVal b As Short, ByVal c As Short) As Short
    Private Declare Function am_option_set_window_preview_handle Lib "amsip" Alias "am_option_set_window_preview_handle" (ByVal a As IntPtr, ByVal b As Short, ByVal c As Short) As Short

    Private Declare Function am_option_select_camera Lib "amsip" Alias "am_option_select_camera" (ByVal a As Short) As Short
    Private Declare Function am_option_find_camera Lib "amsip" Alias "am_option_find_camera" (ByRef a As amsipapi.AM_CAMERA) As Short

    Private Declare Function am_option_enable_preview Lib "amsip" Alias "am_option_enable_preview" (ByVal c As Short) As Short
    Private Declare Function am_video_codec_attr_modify Lib "amsip" Alias "am_video_codec_attr_modify" (ByRef a As amsipapi.AM_VIDEO_CODEC_ATTR) As Short
    Private Declare Function am_video_codec_info_modify Lib "amsip" Alias "am_video_codec_info_modify" (ByRef a As amsipapi.AM_VIDEO_CODEC_INFO, ByVal b As Short) As Short

    Private Declare Function am_message_answer Lib "amsip" Alias "am_message_answer" (ByVal a As Short, ByVal b As Short) As Short

    Private Declare Function am_event_get Lib "amsip" Alias "am_event_get" (ByRef a As amsipapi.EXOSIP_EVENT) As Short
    Private Declare Function am_event_release Lib "amsip" Alias "am_event_release" (ByRef a As amsipapi.EXOSIP_EVENT) As Short

    Private Declare Function am_message_get_header Lib "amsip" Alias "am_message_get_header" (ByVal msg As System.IntPtr, ByVal headername As String, ByRef mheader As amsipapi.AM_HEADER) As Short
    Private Declare Function am_message_get_bodyinfo Lib "amsip" Alias "am_message_get_bodyinfo" (ByVal msg As System.IntPtr, ByVal attachement_idx As Integer, ByRef mbodyinfo As amsipapi.AM_BODYINFO) As Short
    Private Declare Function am_message_release_bodyinfo Lib "amsip" Alias "am_message_release_bodyinfo" (ByRef mbodyinfo As amsipapi.AM_BODYINFO) As Short

    Private Declare Function am_message_get_messageinfo Lib "amsip" Alias "am_message_get_messageinfo" (ByVal msg As System.IntPtr, ByRef minfo As amsipapi.AM_MESSAGEINFO) As Short

#End If

    Public local_config As AM_CONFIG = Nothing

    Public initialized As Integer = 0

    Public rid As Integer = 0
    Public rid_code As Integer = 0
    Public rid_status As String = "Please register!"

    Public global_status As String = "Pls start a Call"

    Public pending_calls As System.Collections.Hashtable = New Hashtable
    Public pending_dialogs As System.Collections.Hashtable = New Hashtable

    Public Function API_am_config(ByVal sip_username As String, ByVal sip_password As String, ByVal sip_domain_realm As String) As Integer

        If (local_config.username = sip_username And local_config.password = sip_password And local_config.domain_realm = sip_domain_realm) Then
            Return 0
        End If
        local_config = New AM_CONFIG
        local_config.username = sip_username
        local_config.password = sip_password
        local_config.domain_realm = sip_domain_realm

        local_config.proxy = "sip:" + sip_domain_realm
        local_config.identity = "sip:" + sip_username + "@" + sip_domain_realm
        Return 0
    End Function

    Public Function API_save_am_config(ByVal file_cfg As String) As Integer
        Return local_config.Save(file_cfg)
    End Function

    Public Function API_load_am_config(ByVal file_cfg As String) As Integer

        Dim cfg As New AM_CONFIG

        If (File.Exists(file_cfg) = True) Then
            'Deserialize text file to a new object.
            Dim objStreamReader As New StreamReader(file_cfg)
            Dim x As New XmlSerializer(cfg.GetType)
            cfg = x.Deserialize(objStreamReader)
            objStreamReader.Close()
        End If

        If (cfg.username = Nothing Or cfg.password = Nothing Or cfg.domain_realm = Nothing) Then
            Return -1
        End If

        local_config = cfg
        Return 0

    End Function

    Public Function API_am_init(ByVal handle As IntPtr, ByVal handlePreview As IntPtr) As Integer
        Dim Ret As Integer

        If (initialized = 0) Then

            initialized = 1

            Ret = am_init("antisip-vbamsip", local_config.debug_level)

            If (Ret < 0) Then
                initialized = 0
                rid_status = "Failed to init amsip, Pls restart!"
                Return Ret
            End If

            am_option_enable_optionnal_encryption(0)
            am_option_set_window_handle(handle, 0, 0)
            am_option_set_window_preview_handle(handlePreview, 176, 144)

            If (local_config.stun_server <> Nothing) Then
                Ret = am_option_enable_turn_server(local_config.stun_server, 1)
                'Ret = am_option_enable_stun_server(local_config.stun_server, 1)
            End If

            Ret = am_network_start(local_config.protocol, local_config.sip_port)

            If (Ret < 0) Then
                Ret = am_network_start(local_config.protocol, 0)
            End If

            If (Ret < 0) Then
                rid_status = "Failed to init SIP socket, Pls restart!"
                Return Ret
            End If

            am_option_set_password("", local_config.username, local_config.password)

            If (local_config.AEC_on = True) Then
                am_option_enable_echo_canceller(1, 128, 2048)
            Else
                am_option_enable_echo_canceller(0, 128, 2048)
            End If

            Dim codec_info As amsipapi.AM_VIDEO_CODEC_INFO
            codec_info = New amsipapi.AM_VIDEO_CODEC_INFO

            codec_info.enable = 1
            codec_info.name = "H264"
            codec_info.payload = 114
            codec_info.freq = 90000
            codec_info.mode = 1
            codec_info.vbr = 0 '61440 '0xF000
            codec_info.cng = 0
            codec_info.vad = 0
            codec_info.dtx = 0
            Ret = am_video_codec_info_modify(codec_info, 0)
            If (Ret < 0) Then
                Return -1
            End If

            codec_info.enable = 1
            codec_info.name = "H263-1998"
            codec_info.payload = 115
            codec_info.freq = 90000
            codec_info.mode = 0
            codec_info.vbr = 0
            codec_info.cng = 0
            codec_info.vad = 0
            codec_info.dtx = 0
            Ret = am_video_codec_info_modify(codec_info, 1)
            If (Ret < 0) Then
                Return -1
            End If
            am_option_enable_preview(1)

        Else
            rid = 0
            rid_code = 0
            rid_status = "Please register!"
            global_status = "Pls start a Call"

            Ret = am_reset("antisip-vbamsip", local_config.debug_level)

            If (Ret < 0) Then
                rid_status = "Failed to reset amsip, Pls restart!"
                Return Ret
            End If

            am_option_enable_optionnal_encryption(0)
            am_option_set_window_handle(handle, 0, 0)
            am_option_set_window_preview_handle(handlePreview, 176, 144)

            If (local_config.stun_server <> Nothing) Then
                Ret = am_option_enable_turn_server(local_config.stun_server, 1)
                'Ret = am_option_enable_stun_server(local_config.stun_server, 1)
            End If

            Ret = am_network_start(local_config.protocol, local_config.sip_port)

            If (Ret < 0) Then
                Ret = am_network_start(local_config.protocol, 0)
            End If

            If (Ret < 0) Then
                rid_status = "Failed to init SIP socket, Pls restart!"
                Return Ret
            End If

            am_option_set_password("", local_config.username, local_config.password)

            If (local_config.AEC_on = True) Then
                am_option_enable_echo_canceller(1, 128, 4096)
            Else
                am_option_enable_echo_canceller(0, 128, 4096)
            End If

            Dim codec_info As amsipapi.AM_VIDEO_CODEC_INFO
            codec_info = New amsipapi.AM_VIDEO_CODEC_INFO

            codec_info.enable = 1
            codec_info.name = "H264"
            codec_info.payload = 114
            codec_info.freq = 90000
            codec_info.mode = 1
            codec_info.vbr = 0 '61440 '0xF000
            codec_info.cng = 0
            codec_info.vad = 0
            codec_info.dtx = 0
            Ret = am_video_codec_info_modify(codec_info, 0)
            If (Ret < 0) Then
                Return -1
            End If

            codec_info.enable = 1
            codec_info.name = "H263-1998"
            codec_info.payload = 115
            codec_info.freq = 90000
            codec_info.mode = 0
            codec_info.vbr = 0
            codec_info.cng = 0
            codec_info.vad = 0
            codec_info.dtx = 0
            Ret = am_video_codec_info_modify(codec_info, 1)
            If (Ret < 0) Then
                Return -1
            End If

            am_option_enable_preview(1)

        End If
        Return 0
    End Function

    Public Function API_am_quit() As Integer
        If (initialized = 0) Then
            Return -1
        End If

        am_quit()
        Return 0
    End Function

    Public Function API_am_option_find_in_sound_card(ByRef sndcard As AM_SNDCARD) As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Return am_option_find_in_sound_card(sndcard)
    End Function

    Public Function API_am_option_find_out_sound_card(ByRef sndcard As AM_SNDCARD) As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Return am_option_find_out_sound_card(sndcard)
    End Function

    Public Function API_am_option_find_camera(ByRef camera As AM_CAMERA) As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Return am_option_find_camera(camera)
    End Function

    Public Function API_am_video_codec_attr_modify(ByVal upload_bandwidth As Integer, ByVal download_bandwidth As Integer) As Integer
        Dim Ret As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Dim codec_attr As amsipapi.AM_VIDEO_CODEC_ATTR
        codec_attr = New amsipapi.AM_VIDEO_CODEC_ATTR

        codec_attr.ptime = 0
        codec_attr.maxptime = 0
        codec_attr.upload_bandwidth = upload_bandwidth
        codec_attr.download_bandwidth = download_bandwidth
        Ret = am_video_codec_attr_modify(codec_attr)
        If (Ret < 0) Then
            Return -1
        End If
        Return 0
    End Function

    Public Function API_am_option_select_in_sound_card(ByVal card As Short) As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Return am_option_select_in_sound_card(card)
    End Function

    Public Function API_am_option_select_out_sound_card(ByVal card As Short) As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Return am_option_select_out_sound_card(card)
    End Function

    Public Function API_am_option_select_camera(ByVal card As Short) As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Return am_option_select_camera(card)
    End Function

    Public Function API_am_option_enable_echo_canceller(ByVal enable_aec As Boolean) As Integer
        If (initialized = 0) Then
            Return -1
        End If

        If (enable_aec = True) Then
            am_option_enable_echo_canceller(1, 128, 2048)
        Else
            am_option_enable_echo_canceller(0, 128, 2048)
        End If

        Return 0
    End Function

    Public Function API_am_option_enable_preview(ByVal enable_display As Boolean) As Integer
        If (initialized = 0) Then
            Return -1
        End If

        If (enable_display = True) Then
            am_option_enable_preview(1)
        Else
            am_option_enable_preview(0)
        End If

        Return 0
    End Function

    Public Function API_am_register() As Integer

        If (initialized = 0) Then
            Return -1
        End If

        If (rid = -1) Then
            rid = 0
        End If
        If (rid <= 0) Then

            rid = am_register_start(local_config.identity, local_config.proxy, 900, 1800)
            If (rid < 0) Then
                Return rid
            End If

            rid_status = "Registring..."
        End If

        Return rid
    End Function

    Public Function API_am_unregister() As Integer

        Dim Ret As Integer

        If (initialized = 0) Then
            Return -1
        End If

        rid = -1
        Ret = am_register_send_star(local_config.identity, local_config.proxy)
        If (Ret < 0) Then
            rid_status = "Failed to unregister!"
        End If
        'for checking unregistration....
        rid_status = "unregistration pending..."

    End Function

    Public Function API_am_session_start(ByVal sip_dest As String) As Integer

        Dim cid As Integer
        If (initialized = 0) Then
            Return -1
        End If

        cid = am_session_start_with_video(local_config.identity, sip_dest, local_config.proxy, "")
        If (cid < 0) Then
            Return cid
        End If

        Dim phone_call As AM_CALL
        phone_call = FindCall(cid)
        phone_call.remote_address = sip_dest

        Return cid
    End Function

    Public Function API_am_session_stop(ByVal acid As Integer, ByVal adid As Integer, ByVal sip_code As Integer) As Integer

        Dim Ret As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Ret = am_session_stop(acid, adid, sip_code)
        If (Ret >= 0) Then
            global_status = "Call Terminated"

            Dim phone_call As AM_CALL
            Dim phone_dialog As AM_DIALOG

            If (pending_calls.Contains(acid) = True) Then
                phone_call = pending_calls(acid)
                phone_call.cid_code = 0
                phone_call.cid_status = "Call Terminated: BYE sent"
                phone_call.state = 3
            End If
            If (pending_dialogs.Contains(adid) = True) Then
                phone_dialog = pending_dialogs(adid)
                phone_dialog.cid_code = 0
                phone_dialog.cid_status = "Call Terminated: BYE sent"
                phone_dialog.state = 3
            End If

            Return Ret
        End If

        Return Ret
    End Function

    Public Function API_am_session_answer(ByVal adid As Integer) As Integer

        Dim Ret As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Dim phone_call As AM_CALL
        Dim phone_dialog As AM_DIALOG
        If (pending_dialogs.Contains(adid) = False) Then
            Return -1
        End If

        phone_dialog = pending_dialogs(adid)

        If (phone_dialog.state = 1) Then
            Ret = am_session_answer(phone_dialog.tid, adid, 200, 1)
            If (Ret >= 0) Then
                phone_dialog.state = 2
                global_status = "Call answered"
                If (pending_calls.Contains(phone_dialog.cid) = True) Then
                    phone_call = pending_calls(phone_dialog.cid)
                    phone_call.state = 2
                End If

                Return Ret
            End If
        End If

        Return -1
    End Function

    Public Function API_am_session_hold(ByVal adid As Integer) As Integer

        Dim Ret As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Dim phone_dialog As AM_DIALOG
        If (pending_dialogs.Contains(adid) = False) Then
            Return -1
        End If

        phone_dialog = pending_dialogs(adid)

        If (phone_dialog.audio_sendrecv = 0) Then
            Ret = am_session_hold(adid, local_config.hold_file)
            If (Ret >= 0) Then
                phone_dialog.audio_sendrecv = 1
                global_status = "Trying to put off hold"
                Return Ret
            End If
        ElseIf (phone_dialog.audio_sendrecv = 1) Then
            Ret = am_session_off_hold(adid)
            If (Ret >= 0) Then
                phone_dialog.audio_sendrecv = 0
                global_status = "Trying to put off hold"
                Return Ret
            End If
        End If

        Return -1
    End Function


    Public Structure AM_STRING256
        <VBFixedString(256), System.Runtime.InteropServices.MarshalAs(System.Runtime.InteropServices.UnmanagedType.ByValTStr, SizeConst:=256)> Public uri As String
    End Structure

    Public Function API_am_session_get_referto(ByVal adid As Integer) As String

        Dim uri As AM_STRING256 = New AM_STRING256
        Dim Ret As Integer

        Ret = am_session_get_referto(adid, uri, 256)
        If (Ret >= 0) Then
            Return uri.uri
        End If

        Return Nothing
    End Function

    Public Function API_am_session_refer(ByVal adid As Integer, ByVal refer_to As String) As Integer

        Dim Ret As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Dim phone_dialog As AM_DIALOG
        If (pending_dialogs.Contains(adid) = False) Then
            Return -1
        End If

        phone_dialog = pending_dialogs(adid)

        Ret = am_session_refer(adid, refer_to, local_config.identity)
        If (Ret >= 0) Then
            phone_dialog.audio_sendrecv = 1
            phone_dialog.cid_status = "Transfering to " + refer_to
            Return Ret
        End If

        Return -1
    End Function

    Public Function API_am_session_mute(ByVal adid As Integer) As Integer

        Dim Ret As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Dim phone_dialog As AM_DIALOG
        If (pending_dialogs.Contains(adid) = False) Then
            Return -1
        End If

        phone_dialog = pending_dialogs(adid)

        If (phone_dialog.muted = 0) Then
            Ret = am_session_mute(adid)
            If (Ret >= 0) Then
                phone_dialog.muted = 1
                global_status = "Call Muted"
                Return Ret
            End If
        ElseIf (phone_dialog.muted = 1) Then
            Ret = am_session_unmute(adid)
            If (Ret >= 0) Then
                phone_dialog.muted = 0
                global_status = "Call Activated"
                Return Ret
            End If
        End If

        Return -1
    End Function

    Public Function API_am_session_add_video(ByVal adid As Integer) As Integer

        Dim Ret As Integer
        If (initialized = 0) Then
            Return -1
        End If

        Dim phone_dialog As AM_DIALOG
        If (pending_dialogs.Contains(adid) = False) Then
            Return -1
        End If

        phone_dialog = pending_dialogs(adid)

        If (phone_dialog.video_sendrecv = -1) Then
            Ret = am_session_add_video(adid)
            If (Ret >= 0) Then
                phone_dialog.video_sendrecv = 1
                global_status = "Opening Video"
                Return Ret
            End If
        ElseIf (phone_dialog.video_sendrecv = 1) Then
            Ret = am_session_add_video(adid)
            If (Ret >= 0) Then
                phone_dialog.video_sendrecv = 1
                global_status = "Restarting video..."
                Return Ret
            End If
        End If

        Return -1
    End Function

    Public Function API_am_session_get_audio_statistics(ByVal did As Short, ByRef audio_stats As AM_AUDIO_STATS) As Integer
        Return am_session_get_audio_statistics(did, audio_stats)
    End Function

    Public Function API_process_events() As Integer

        Dim Ret As Integer
        Dim evt As amsipapi.EXOSIP_EVENT
        Dim minfo As amsipapi.AM_MESSAGEINFO

        If (initialized = 0) Then
            Return -1
        End If

        evt = New amsipapi.EXOSIP_EVENT

        Ret = am_event_get(evt)
        If (Ret = 50) Then
            Return -1
        End If

        If (evt.rid > 0 And rid = -1) Then
            ' wait for end of unregistration
            minfo = New amsipapi.AM_MESSAGEINFO
            am_message_get_messageinfo(evt.response, minfo)

            If (evt.type = 2) Then
                If (minfo.answer_code > 0) Then
                    rid_code = minfo.answer_code
                    rid_status = "Failed: " + minfo.answer_code.ToString() + " " + minfo.reason
                Else
                    rid_code = 0
                    rid_status = "Failed: check your proxy?"
                End If
            ElseIf (evt.type = 1) Then
                rid = 0
                rid_code = 0
                rid_status = "Registration Deleted on server!"
            End If
            If (evt.type = 1) Then
            End If
            am_event_release(evt)
            Return Ret
        End If

        If (evt.rid > 0) Then
            minfo = New amsipapi.AM_MESSAGEINFO
            am_message_get_messageinfo(evt.response, minfo)

            If (evt.type = 2) Then
                If (minfo.answer_code > 0) Then
                    rid_code = minfo.answer_code
                    rid_status = "Failed: " + minfo.answer_code.ToString() + " " + minfo.reason
                Else
                    rid_code = 0
                    rid_status = "Failed: check your proxy?"
                End If
            ElseIf (evt.type = 1) Then
                rid_status = minfo.answer_code.ToString() + " " + minfo.reason + " -> Registered."
                rid_code = 200
            End If

            am_event_release(evt)
            Return 0
        ElseIf (evt.cid = 0 And evt.did = 0 And evt.nid = 0 And evt.sid = 0) Then

            If (evt.type = 27) Then

                minfo = New amsipapi.AM_MESSAGEINFO
                am_message_get_messageinfo(evt.request, minfo)

                'EXOSIP_MESSAGE_NEW
                If (minfo.method = "MESSAGE") Then
                    am_message_answer(evt.tid, 200)
                ElseIf (minfo.method = "PING") Then
                    am_message_answer(evt.tid, 200)
                ElseIf (minfo.method = "REFER") Then

                    am_message_answer(evt.tid, 200)

                ElseIf (minfo.method = "OPTIONS") Then
                    am_message_answer(evt.tid, 200)
                ElseIf (minfo.method = "REGISTER") Then
                    am_message_answer(evt.tid, 405)
                ElseIf (minfo.method = "FETCH") Then
                    am_message_answer(evt.tid, 405)
                ElseIf (minfo.method = "UPDATE") Then
                    am_message_answer(evt.tid, 405)
                ElseIf (minfo.method = "INFO") Then
                    am_message_answer(evt.tid, 405)
                Else
                    am_message_answer(evt.tid, 501)
                End If

                am_event_release(evt)
                Return 0
            End If

            am_event_release(evt)
            Return 0

        ElseIf (evt.cid > 0) Then
            Dim phone_call As AM_CALL
            Dim phone_dialog As AM_DIALOG

            minfo = New amsipapi.AM_MESSAGEINFO
            am_message_get_messageinfo(evt.response, minfo)

            phone_call = FindCall(evt)
            phone_dialog = FindDialog(evt)

            If (phone_call.state <= 1) Then 'not established

                If (phone_call.remote_address.Length <= 0) Then
                    phone_call.remote_address = minfo.sip_from
                End If
                If (phone_dialog.remote_address.Length <= 0) Then
                    phone_dialog.remote_address = phone_call.remote_address
                End If

                If (evt.type = 7 Or evt.type = 11 Or evt.type = 12 Or evt.type = 13 Or evt.type = 14) Then

                    If (minfo.answer_code = 401 Or minfo.answer_code = 407) Then
                        phone_call.cid_code = minfo.answer_code
                        phone_call.cid_status = minfo.answer_code.ToString() + minfo.reason
                        phone_call.state = 1
                        phone_dialog.cid_code = minfo.answer_code
                        phone_dialog.cid_status = minfo.answer_code.ToString() + minfo.reason
                        phone_dialog.state = 1
                        am_event_release(evt)
                        Return 0
                    ElseIf (minfo.answer_code >= 300 And minfo.answer_code <= 399) Then
                        phone_call.cid_code = minfo.answer_code
                        phone_call.cid_status = minfo.answer_code.ToString() + minfo.reason
                        phone_call.state = 1
                        phone_dialog.cid_code = minfo.answer_code
                        phone_dialog.cid_status = minfo.answer_code.ToString() + minfo.reason
                        phone_dialog.state = 1
                        am_event_release(evt)
                        Return 0
                    End If

                    If (evt.type = 7) Then
                        phone_call.cid_code = minfo.answer_code
                        phone_call.cid_status = "Call Terminated: No answer -> check your proxy"
                        phone_call.state = 3
                        phone_dialog.cid_code = 0
                        phone_dialog.cid_status = "Call Terminated: No answer -> check your proxy"
                        phone_dialog.state = 3
                    Else
                        phone_call.cid_code = minfo.answer_code
                        phone_call.cid_status = "Call Terminated: " + minfo.answer_code.ToString() + minfo.reason
                        phone_call.state = 3
                        phone_dialog.cid_code = 0
                        phone_dialog.cid_status = "Call Terminated: " + minfo.answer_code.ToString() + minfo.reason
                        phone_dialog.state = 3
                    End If
                    am_event_release(evt)
                    Return 0
                End If
            End If

            If (evt.type = 25) Then
                'BYE was received for call
                phone_call.cid_code = 0
                phone_call.cid_status = "Call Terminated: BYE sent"
                phone_call.state = 3
                phone_dialog.cid_code = 0
                phone_dialog.cid_status = "Call Terminated: BYE received"
                phone_dialog.state = 3

                global_status = "Call terminated"
                am_event_release(evt)
                Return 0
            ElseIf (evt.type = 26) Then
                'ressource released
                pending_calls.Remove(evt.cid)
                If (phone_dialog.state > -2) Then
                    pending_dialogs.Remove(evt.did)
                End If

                global_status = "Pls start a Call"
                am_event_release(evt)
                Return 0
            End If

            'check when call is pending if it's entering confirmed state
            If (phone_call.state <= 1) Then 'not established

                If (evt.type = 8 Or evt.type = 9) Then
                    phone_call.state = 1
                    If (minfo.answer_code > 0) Then
                        phone_call.cid_code = minfo.answer_code
                        phone_dialog.cid_code = minfo.answer_code
                        phone_call.cid_status = minfo.reason
                        phone_dialog.cid_status = minfo.reason
                        am_event_release(evt)
                        Return 0
                    End If
                ElseIf (evt.type = 10) Then
                    phone_call.state = 2
                    phone_dialog.state = 2
                    phone_call.cid_code = minfo.answer_code
                    phone_dialog.cid_code = minfo.answer_code
                    phone_call.cid_status = minfo.reason
                    phone_dialog.cid_status = minfo.reason
                    am_event_release(evt)
                    Return 0
                End If
            End If

            If (evt.type = 5) Then
                'receive new INVITE
                Dim mreq As amsipapi.AM_MESSAGEINFO
                mreq = New amsipapi.AM_MESSAGEINFO
                am_message_get_messageinfo(evt.request, mreq)

                'search for a Replace header
                Dim cid_replaces As Integer
                cid_replaces = am_session_find_by_replaces(evt.request)
                If (cid_replaces > 0) Then
                    Me.API_am_session_stop(cid_replaces, 0, 486)
                    phone_dialog.remote_address = mreq.sip_from
                    phone_call.remote_address = mreq.sip_from
                    phone_call.state = 1
                    phone_dialog.state = 1
                    phone_call.cid_code = 200
                    phone_dialog.cid_code = 200
                    If (phone_call.call_idx = -1) Then
                        phone_call.cid_status = "Call Rejected (Busy)"
                        phone_dialog.cid_status = "Call Rejected (Busy)"

                        am_session_answer(evt.tid, evt.did, 486, 0)
                    Else
                        phone_call.cid_status = "Call Accepted"
                        phone_dialog.cid_status = "Call Accepted"

                        am_session_answer(evt.tid, evt.did, 180, 0)
                        phone_call.state = 2
                        phone_dialog.state = 2
                        am_session_answer(evt.tid, evt.did, 200, 1)
                    End If
                    am_event_release(evt)
                    Return 0
                End If

                phone_dialog.remote_address = mreq.sip_from
                phone_call.remote_address = mreq.sip_from
                phone_call.state = 1
                phone_dialog.state = 1
                phone_call.cid_code = 200
                phone_dialog.cid_code = 200
                If (phone_call.call_idx = -1) Then
                    phone_call.cid_status = "Call Rejected (Busy)"
                    phone_dialog.cid_status = "Call Rejected (Busy)"

                    am_session_answer(evt.tid, evt.did, 486, 0)
                Else
                    phone_call.cid_status = "Call Accepted"
                    phone_dialog.cid_status = "Call Accepted"

                    am_session_answer(evt.tid, evt.did, 180, 0)
                    'phone_call.state = 2
                    'phone_dialog.state = 2
                    'am_session_answer(evt.tid, evt.did, 200, 1)
                End If
                am_event_release(evt)
                Return 0
            End If

            If (phone_call.state = 2) Then 'established

                If (evt.type = 15) Then
                    'receive ACK
                    am_event_release(evt)
                    Return 0
                End If

                If (evt.type = 6) Then
                    'receive INVITE within call
                    am_session_answer(evt.tid, evt.did, 200, 1)
                    am_event_release(evt)
                    Return 0
                End If
            End If

            'EXOSIP_CALL_MESSAGE_NEW
            If (evt.type = 18) Then
                Dim mreq As amsipapi.AM_MESSAGEINFO
                mreq = New amsipapi.AM_MESSAGEINFO
                am_message_get_messageinfo(evt.request, mreq)

                If (mreq.method = "BYE") Then
                    'answered internally
                    'am_session_answer_request(evt.tid, evt.did, 200)
                ElseIf (mreq.method = "MESSAGE") Then
                    am_session_answer_request(evt.tid, evt.did, 200)
                ElseIf (mreq.method = "UPDATE") Then
                    am_session_answer_request(evt.tid, evt.did, 200)
                ElseIf (mreq.method = "INFO") Then
                    am_session_answer_request(evt.tid, evt.did, 200)
                ElseIf (mreq.method = "PING") Then
                    am_session_answer_request(evt.tid, evt.did, 200)
                ElseIf (mreq.method = "REFER") Then
                    Dim head As amsipapi.AM_HEADER
                    head = New amsipapi.AM_HEADER

                    head.index = 0
                    Ret = am_message_get_header(evt.request, "refer-to", head)
                    If (Ret < 0) Then
                        'Missing Refer-To header
                        am_session_answer_request(evt.tid, evt.did, 400)
                        am_event_release(evt)
                        Return -1
                    End If

                    Dim cid_refer As Integer = Me.API_am_session_start(head.header_value)
                    If (cid_refer <= 0) Then
                        am_session_answer_request(evt.tid, evt.did, 400)
                        am_event_release(evt)
                        Return 0
                    End If

                    am_session_answer_request(evt.tid, evt.did, 202)
                    am_event_release(evt)
                    Return 0
                ElseIf (mreq.method = "NOTIFY") Then

                    If (phone_dialog.refer_did < 0) Then
                        am_session_answer_request(evt.tid, evt.did, 489)
                        am_event_release(evt)
                        Return 0
                    End If

                    am_session_answer_request(evt.tid, evt.did, 200)

                    Dim binfo As AM_BODYINFO = New AM_BODYINFO
                    Ret = am_message_get_bodyinfo(evt.request, 0, binfo)

                    ' check if body contains 200 -> hang call
                    If (Ret >= 0) Then
                        Dim sipfrag As String
                        'sipfrag = System.Runtime.InteropServices.Marshal.PtrToStringUni(binfo.attachment)
                        sipfrag = System.Runtime.InteropServices.Marshal.PtrToStringAnsi(binfo.attachment)

                        If (sipfrag.Contains("SIP/2.0 2") = True) Then
                            If (phone_dialog.refer_did > 0) Then

                                API_am_session_stop(phone_dialog.refer_cid, phone_dialog.refer_did, 500)

                                phone_dialog.refer_cid = 0
                                phone_dialog.refer_did = 0

                            End If

                            API_am_session_stop(phone_dialog.cid, phone_dialog.did, 500)
                            phone_dialog.cid_status = "Transfer terminated (" + sipfrag + ")"
                            phone_call.cid_status = "Transfer terminated (" + sipfrag + ")"
                        ElseIf (sipfrag.Contains("SIP/2.0 1") = True) Then
                            phone_dialog.cid_status = "Transfer pending (" + sipfrag + ")"
                            phone_call.cid_status = "Transfer pending (" + sipfrag + ")"
                        Else
                            phone_dialog.cid_status = "Transfer interupted (" + sipfrag + ")"
                            phone_call.cid_status = "Transfer interupted (" + sipfrag + ")"
                        End If
                        am_message_release_bodyinfo(binfo)
                    End If

                ElseIf (mreq.method = "OPTIONS") Then
                    am_session_answer_request(evt.tid, evt.did, 200)
                ElseIf (mreq.method = "REGISTER") Then
                    am_session_answer_request(evt.tid, evt.did, 405)
                ElseIf (mreq.method = "FETCH") Then
                    am_session_answer_request(evt.tid, evt.did, 405)
                Else
                    am_session_answer_request(evt.tid, evt.did, 501)
                End If

                am_event_release(evt)
                Return 0
            End If

            am_event_release(evt)
            Return 0
        End If

        am_event_release(evt)
        Return Ret
    End Function

    Public Function FindDialog(ByRef evt As amsipapi.EXOSIP_EVENT) As AM_DIALOG

        Dim phone_dialog As AM_DIALOG

        If (pending_dialogs.Contains(evt.did) = True) Then
            phone_dialog = pending_dialogs(evt.did)
            Return phone_dialog
        End If

        phone_dialog = New AM_DIALOG
        phone_dialog.tid = evt.tid
        phone_dialog.cid = evt.cid
        phone_dialog.did = evt.did
        phone_dialog.remote_address = ""
        If (evt.did <= 0) Then
            'no dialog
            phone_dialog.state = -2
            Return phone_dialog
        End If

        phone_dialog.state = -1
        pending_dialogs.Add(evt.did, phone_dialog)
        Return phone_dialog
    End Function

    Public Function FindCall(ByRef acid As Integer) As AM_CALL
        Dim phone_call As AM_CALL
        phone_call = New AM_CALL
        phone_call.tid = 0
        phone_call.cid = acid
        phone_call.did = 0
        phone_call.remote_address = ""
        phone_call.state = -1

        'assign a call index between 1 and 4
        If (GetCallIndex(1) < 0) Then
            phone_call.call_idx = 1
        ElseIf (GetCallIndex(2) < 0) Then
            phone_call.call_idx = 2
        ElseIf (GetCallIndex(3) < 0) Then
            phone_call.call_idx = 3
        ElseIf (GetCallIndex(4) < 0) Then
            phone_call.call_idx = 4
        End If

        pending_calls.Add(acid, phone_call)
        Return phone_call
    End Function

    Public Function FindCall(ByRef evt As amsipapi.EXOSIP_EVENT) As AM_CALL

        Dim phone_call As AM_CALL

        If (pending_calls.Contains(evt.cid) = True) Then
            phone_call = pending_calls(evt.cid)
            Return phone_call
        End If

        phone_call = New AM_CALL
        phone_call.tid = evt.tid
        phone_call.cid = evt.cid
        phone_call.did = evt.did
        phone_call.remote_address = ""
        phone_call.state = -1

        'assign a call index between 1 and 4
        phone_call.call_idx = -1
        If (GetCallIndex(1) < 0) Then
            phone_call.call_idx = 1
        ElseIf (GetCallIndex(2) < 0) Then
            phone_call.call_idx = 2
        ElseIf (GetCallIndex(3) < 0) Then
            phone_call.call_idx = 3
        ElseIf (GetCallIndex(4) < 0) Then
            phone_call.call_idx = 4
        End If

        pending_calls.Add(evt.cid, phone_call)
        Return phone_call
    End Function

    Public Function GetCallIndex(ByVal idx As Integer) As Integer
        Dim phone_call As AM_CALL
        For Each dic_entry As DictionaryEntry In pending_calls
            phone_call = dic_entry.Value
            If (phone_call.call_idx = idx) Then
                Return phone_call.cid
            End If
        Next dic_entry
        Return -1
    End Function

    Public Function GetPendingCall() As Integer
        Dim phone_call As AM_CALL
        For Each dic_entry As DictionaryEntry In pending_calls
            phone_call = dic_entry.Value
            If (phone_call.state < 2) Then
                Return phone_call.cid
            End If
        Next dic_entry

        Return -1
    End Function

    Public Function GetActiveCall() As Integer
        Dim phone_call As AM_CALL
        For Each dic_entry As DictionaryEntry In pending_calls
            phone_call = dic_entry.Value
            If (phone_call.state = 2) Then
                Return phone_call.cid
            End If
        Next dic_entry

        Return -1
    End Function

    Public Function GetPendingDialog(ByVal acid As Integer) As Integer
        Dim phone_dialog As AM_DIALOG
        For Each dic_entry As DictionaryEntry In pending_dialogs
            phone_dialog = dic_entry.Value
            If (phone_dialog.state < 2 And acid = phone_dialog.cid) Then
                Return phone_dialog.did
            End If
        Next dic_entry

        Return -1
    End Function

    Public Function GetActiveDialog(ByVal acid As Integer) As Integer
        Dim phone_dialog As AM_DIALOG
        For Each dic_entry As DictionaryEntry In pending_dialogs
            phone_dialog = dic_entry.Value
            If (phone_dialog.state = 2 And acid = phone_dialog.cid) Then
                Return phone_dialog.did
            End If
        Next dic_entry

        Return -1
    End Function

    Public Function GetTerminatedDialog(ByVal acid As Integer) As Integer
        Dim phone_dialog As AM_DIALOG
        For Each dic_entry As DictionaryEntry In pending_dialogs
            phone_dialog = dic_entry.Value
            If (phone_dialog.state = 3 And acid = phone_dialog.cid) Then
                Return phone_dialog.did
            End If
        Next dic_entry

        Return -1
    End Function

    Public Function GetPendingDialog() As Integer
        Dim phone_dialog As AM_DIALOG
        For Each dic_entry As DictionaryEntry In pending_dialogs
            phone_dialog = dic_entry.Value
            If (phone_dialog.state < 2) Then
                Return phone_dialog.did
            End If
        Next dic_entry

        Return -1
    End Function

    Public Function GetActiveDialog() As Integer
        Dim phone_dialog As AM_DIALOG
        For Each dic_entry As DictionaryEntry In pending_dialogs
            phone_dialog = dic_entry.Value
            If (phone_dialog.state = 2) Then
                Return phone_dialog.did
            End If
        Next dic_entry

        Return -1
    End Function

End Class

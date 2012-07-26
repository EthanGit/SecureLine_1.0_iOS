
#ifndef AMSIP_INTERNAL_H
#define AMSIP_INTERNAL_H

extern struct eXosip_t *amsip_eXosip;

#define eXosip_quit() eXosip_quit(amsip_eXosip)
#define eXosip_init() eXosip_init(amsip_eXosip)
#define eXosip_add_authentication_info(A, B, C, D, E) eXosip_add_authentication_info(amsip_eXosip, A, B, C, D, E)
#define eXosip_set_option(A, B) eXosip_set_option(amsip_eXosip, A, B)
#define eXosip_listen_addr(A, B, C, D, E) eXosip_listen_addr(amsip_eXosip, A, B, C, D, E)

#define eXosip_call_build_initial_invite(A, B, C, D, E) eXosip_call_build_initial_invite(amsip_eXosip, A, B, C, D, E)
#define eXosip_call_send_initial_invite(A) eXosip_call_send_initial_invite(amsip_eXosip, A)
#define eXosip_call_build_answer(A, B, C) eXosip_call_build_answer(amsip_eXosip, A, B, C)
#define eXosip_call_send_answer(A, B, C) eXosip_call_send_answer(amsip_eXosip, A, B, C)
#define eXosip_call_build_ack(A, B) eXosip_call_build_ack(amsip_eXosip, A, B)
#define eXosip_call_send_ack(A, B) eXosip_call_send_ack(amsip_eXosip, A, B)
#define eXosip_call_build_request(A, B, C) eXosip_call_build_request(amsip_eXosip, A, B, C)
#define eXosip_call_send_request(A, B) eXosip_call_send_request(amsip_eXosip, A, B)
#define eXosip_call_build_info(A, B) eXosip_call_build_info(amsip_eXosip, A, B)
#define eXosip_call_build_refer(A, B, C) eXosip_call_build_refer(amsip_eXosip, A, B, C)
#define eXosip_call_get_referto(A, B, C) eXosip_call_get_referto(amsip_eXosip, A, B, C)
#define eXosip_call_find_by_replaces(A) eXosip_call_find_by_replaces(amsip_eXosip, A)
#define eXosip_call_build_prack(A, B) eXosip_call_build_prack(amsip_eXosip, A, B)
#define eXosip_call_send_prack(A, B) eXosip_call_send_prack(amsip_eXosip, A, B)
#define eXosip_get_remote_sdp_from_tid(A) eXosip_get_remote_sdp_from_tid(amsip_eXosip, A)
#define eXosip_call_terminate(A, B) eXosip_call_terminate(amsip_eXosip, A, B)

#define eXosip_automatic_action() eXosip_automatic_action(amsip_eXosip)
#define eXosip_default_action(A) eXosip_default_action(amsip_eXosip, A)

#define eXosip_set_user_agent(A) eXosip_set_user_agent(amsip_eXosip, A)

#define eXosip_subscribe_build_initial_request(A, B, C, D, E, F) eXosip_subscribe_build_initial_request(amsip_eXosip, A, B, C, D, E, F) 
#define eXosip_subscribe_send_initial_request(A) eXosip_subscribe_send_initial_request(amsip_eXosip, A)
#define eXosip_subscribe_build_refresh_request(A, B) eXosip_subscribe_build_refresh_request(amsip_eXosip, A, B)
#define eXosip_subscribe_send_refresh_request(A, B) eXosip_subscribe_send_refresh_request(amsip_eXosip, A, B)
#define eXosip_subscribe_remove(A) eXosip_subscribe_remove(amsip_eXosip, A)
#define eXosip_insubscription_build_answer(A, B, C) eXosip_insubscription_build_answer(amsip_eXosip, A, B, C)
#define eXosip_insubscription_send_answer(A, B, C) eXosip_insubscription_send_answer(amsip_eXosip, A, B, C)
#define eXosip_insubscription_build_notify(A, B, C, D) eXosip_insubscription_build_notify(amsip_eXosip, A, B, C, D) 
#define eXosip_insubscription_send_request(A, B) eXosip_insubscription_send_request(amsip_eXosip, A, B)
#define eXosip_insubscription_remove(A) eXosip_insubscription_remove(amsip_eXosip, A)

#define eXosip_lock() eXosip_lock(amsip_eXosip) 
#define eXosip_unlock() eXosip_unlock(amsip_eXosip) 
#define eXosip_event_wait(A, B) eXosip_event_wait(amsip_eXosip, A, B)
#define eXosip_event_geteventsocket() eXosip_event_geteventsocket(amsip_eXosip)

#define eXosip_message_build_request(A, B, C, D, E) eXosip_message_build_request(amsip_eXosip, A, B, C, D, E)
#define eXosip_message_send_request(A) eXosip_message_send_request(amsip_eXosip, A)
#define eXosip_message_build_answer(A, B, C) eXosip_message_build_answer(amsip_eXosip, A, B, C)
#define eXosip_message_send_answer(A, B, C) eXosip_message_send_answer(amsip_eXosip, A, B, C)

#define eXosip_options_build_request(A, B, C, D) eXosip_options_build_request(amsip_eXosip, A, B, C, D)
#define eXosip_options_send_request(A) eXosip_options_send_request(amsip_eXosip, A)
#define eXosip_options_build_answer(A, B, C) eXosip_options_build_answer(amsip_eXosip, A, B, C)
#define eXosip_options_send_answer(A, B, C) eXosip_options_send_answer(amsip_eXosip, A, B, C)

#define eXosip_build_publish(A, B, C, D, E, F, G, H) eXosip_build_publish(amsip_eXosip, A, B, C, D, E, F, G, H)
#define eXosip_publish(A, B) eXosip_publish(amsip_eXosip, A, B)

#define eXosip_refer_build_request(A, B, C, D, E) eXosip_refer_build_request(amsip_eXosip, A, B, C, D, E)
#define eXosip_refer_send_request(A) eXosip_refer_send_request(amsip_eXosip, A)

#define eXosip_register_build_initial_register(A, B, C, D, E) eXosip_register_build_initial_register(amsip_eXosip, A, B, C, D, E)
#define eXosip_register_build_register(A, B, C) eXosip_register_build_register(amsip_eXosip, A, B, C)
#define eXosip_register_send_register(A, B) eXosip_register_send_register(amsip_eXosip, A, B)
#define eXosip_register_remove(A) eXosip_register_remove(amsip_eXosip, A)

#define eXosip_clear_authentication_info() eXosip_clear_authentication_info(amsip_eXosip)
#define eXosip_remove_authentication_info(A, B) eXosip_remove_authentication_info(amsip_eXosip, A, B)

#define eXosip_get_remote_sdp(A) eXosip_get_remote_sdp(amsip_eXosip, A)
#define eXosip_get_local_sdp(A) eXosip_get_local_sdp(amsip_eXosip, A)
#define eXosip_get_previous_local_sdp(A) eXosip_get_previous_local_sdp(amsip_eXosip, A)

#define eXosip_guess_localip(A, B, C) eXosip_guess_localip(amsip_eXosip, A, B, C)
#define eXosip_find_free_port(A, B) eXosip_find_free_port(amsip_eXosip, A, B)
#define eXosip_masquerade_contact(A, B) eXosip_masquerade_contact(amsip_eXosip, A, B)

#define eXosip_tls_verify_certificate(A) eXosip_tls_verify_certificate(amsip_eXosip, A)
#define eXosip_set_tls_ctx(A) eXosip_set_tls_ctx(amsip_eXosip, A)

#endif

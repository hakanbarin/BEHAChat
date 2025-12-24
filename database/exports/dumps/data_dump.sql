--
-- PostgreSQL database dump
--

\restrict 6uVolImc03jY6xptKaSGljhDmg12kYKOWA4MqWAKbw5ttcXCDtjTgHXBohuVWo5

-- Dumped from database version 17.7 (Ubuntu 17.7-0ubuntu0.25.10.1)
-- Dumped by pg_dump version 17.7 (Ubuntu 17.7-0ubuntu0.25.10.1)

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Data for Name: users; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.users VALUES (1, 'admin', '$2a$12$LQv3c1yqBWVHxkd0LHAkCOYz6TtxMQJqhN8/LewY5hjLv5yzmC.gC', 0, false, '2025-12-19 14:34:03.225757', '2025-12-19 14:34:03.225757', '2025-12-19 14:34:03.225757');
INSERT INTO public.users VALUES (2, 'moderator', '$2a$12$LQv3c1yqBWVHxkd0LHAkCOYz6TtxMQJqhN8/LewY5hjLv5yzmC.gC', 1, false, '2025-12-19 14:34:03.225757', '2025-12-19 14:34:03.225757', '2025-12-19 14:34:03.225757');
INSERT INTO public.users VALUES (3, 'test_user', '$2a$12$LQv3c1yqBWVHxkd0LHAkCOYz6TtxMQJqhN8/LewY5hjLv5yzmC.gC', 2, false, '2025-12-19 14:34:03.225757', '2025-12-19 14:34:03.225757', '2025-12-19 14:34:03.225757');


--
-- Data for Name: bans; Type: TABLE DATA; Schema: public; Owner: postgres
--



--
-- Data for Name: schema_version; Type: TABLE DATA; Schema: public; Owner: postgres
--

INSERT INTO public.schema_version VALUES (0, '2025-12-19 14:34:03.187807', 'Initial database setup');
INSERT INTO public.schema_version VALUES (1, '2025-12-19 14:34:03.226439', 'Users table created');
INSERT INTO public.schema_version VALUES (2, '2025-12-19 14:34:03.264719', 'Tokens table created');
INSERT INTO public.schema_version VALUES (3, '2025-12-19 14:34:03.301766', 'Bans table created');
INSERT INTO public.schema_version VALUES (4, '2025-12-19 14:34:03.338163', 'Session logs table created');


--
-- Data for Name: session_logs; Type: TABLE DATA; Schema: public; Owner: postgres
--



--
-- Data for Name: tokens; Type: TABLE DATA; Schema: public; Owner: postgres
--



--
-- Name: bans_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.bans_id_seq', 1, false);


--
-- Name: session_logs_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.session_logs_id_seq', 1, false);


--
-- Name: tokens_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.tokens_id_seq', 1, false);


--
-- Name: users_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.users_id_seq', 3, true);


--
-- PostgreSQL database dump complete
--

\unrestrict 6uVolImc03jY6xptKaSGljhDmg12kYKOWA4MqWAKbw5ttcXCDtjTgHXBohuVWo5


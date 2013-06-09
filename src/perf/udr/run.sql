set stats on;

select count(*) gen_rows_message from gen_rows_message(1, 9000000);
select count(*) gen_rows_message_5c from gen_rows_message_5c(1, 9000000);
select count(*) gen_rows_message_nn from gen_rows_message_nn(1, 9000000);
select count(*) gen_rows_message_5c_nn from gen_rows_message_5c_nn(1, 9000000);
select count(*) gen_rows_message_xbig from gen_rows_message_xbig(1, 9000000);
select count(*) gen_rows_message_5c_xbig from gen_rows_message_5c_xbig(1, 9000000);
select count(*) gen_rows_psql from gen_rows_psql(1, 9000000);
select count(*) gen_rows_psql_5c from gen_rows_psql_5c(1, 9000000);

select count(*) copy_message from copy_message(9000000, 'abcdefghij');
select count(*) copy_message_5c from copy_message_5c(9000000, 'abcdefghij');
select count(*) copy_message_nn from copy_message_nn(9000000, 'abcdefghij');
select count(*) copy_message_5c_nn from copy_message_5c_nn(9000000, 'abcdefghij');
select count(*) copy_message_xbig from copy_message_xbig(9000000, 1234567890);
select count(*) copy_message_5c_xbig from copy_message_5c_xbig(9000000, 1234567890);
select count(*) copy_psql from copy_psql(9000000, 'abcdefghij');
select count(*) copy_psql_5c from copy_psql_5c(9000000, 'abcdefghij');

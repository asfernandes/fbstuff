set term !;


create or alter procedure gen_rows_message (
	start_n integer,
	end_n integer
) returns (
	result integer
)
	external name 'perfudr!gen_rows_message'
	engine udr!

create or alter procedure gen_rows_message_nn (
	start_n integer not null,
	end_n integer not null
) returns (
	result integer not null
)
	external name 'perfudr!gen_rows_message'
	engine udr!

create or alter procedure gen_rows_message_5c (
	start_n integer,
	end_n integer
) returns (
	result1 integer,
	result2 integer,
	result3 integer,
	result4 integer,
	result5 integer
)
	external name 'perfudr!gen_rows_message_5c'
	engine udr!

create or alter procedure gen_rows_message_5c_nn (
	start_n integer not null,
	end_n integer not null
) returns (
	result1 integer not null,
	result2 integer not null,
	result3 integer not null,
	result4 integer not null,
	result5 integer not null
)
	external name 'perfudr!gen_rows_message_5c'
	engine udr!

create or alter procedure gen_rows_message_xbig (
	start_n bigint,
	end_n bigint
) returns (
	result bigint
)
	external name 'perfudr!gen_rows_message'
	engine udr!

create or alter procedure gen_rows_message_5c_xbig (
	start_n bigint,
	end_n bigint
) returns (
	result1 bigint,
	result2 bigint,
	result3 bigint,
	result4 bigint,
	result5 bigint
)
	external name 'perfudr!gen_rows_message_5c'
	engine udr!

create or alter procedure copy_message (
	count_n integer,
	input varchar(20)
) returns (
	output varchar(20)
)
	external name 'perfudr!copy_message'
	engine udr!

create or alter procedure copy_message_nn (
	count_n integer not null,
	input varchar(20) not null
) returns (
	output varchar(20) not null
)
	external name 'perfudr!copy_message'
	engine udr!

create or alter procedure copy_message_5c (
	count_n integer,
	input varchar(20)
) returns (
	output1 varchar(20),
	output2 varchar(20),
	output3 varchar(20),
	output4 varchar(20),
	output5 varchar(20)
)
	external name 'perfudr!copy_message_5c'
	engine udr!

create or alter procedure copy_message_5c_nn (
	count_n integer not null,
	input varchar(20) not null
) returns (
	output1 varchar(20) not null,
	output2 varchar(20) not null,
	output3 varchar(20) not null,
	output4 varchar(20) not null,
	output5 varchar(20) not null
)
	external name 'perfudr!copy_message_5c'
	engine udr!

create or alter procedure copy_message_xbig (
	count_n bigint,
	input bigint
) returns (
	output bigint
)
	external name 'perfudr!copy_message'
	engine udr!

create or alter procedure copy_message_5c_xbig (
	count_n bigint,
	input bigint
) returns (
	output1 bigint,
	output2 bigint,
	output3 bigint,
	output4 bigint,
	output5 bigint
)
	external name 'perfudr!copy_message_5c'
	engine udr!


-- TODO: PSQL without


create or alter procedure gen_rows_psql (
	start_n integer,
	end_n integer
) returns (
	result integer
)
as
begin
	while (start_n <= end_n)
	do
	begin
		result = start_n;
		start_n = start_n + 1;
		suspend;
	end
end!


create or alter procedure gen_rows_psql_5c (
	start_n integer,
	end_n integer
) returns (
	result1 integer,
	result2 integer,
	result3 integer,
	result4 integer,
	result5 integer
)
as
begin
	while (start_n <= end_n)
	do
	begin
		result1 = start_n;
		result2 = result1;
		result3 = result1;
		result4 = result1;
		result5 = result1;
		start_n = start_n + 1;
		suspend;
	end
end!


create or alter procedure copy_psql (
	count_n integer,
	input varchar(20)
) returns (
	output varchar(20)
)
as
begin
	output = input;

	while (count_n > 0)
	do
	begin
		count_n = count_n - 1;
		suspend;
	end
end!

create or alter procedure copy_psql_5c (
	count_n integer,
	input varchar(20)
) returns (
	output1 varchar(20),
	output2 varchar(20),
	output3 varchar(20),
	output4 varchar(20),
	output5 varchar(20)
)
as
begin
	output1 = input;
	output2 = input;
	output3 = input;
	output4 = input;
	output5 = input;

	while (count_n > 0)
	do
	begin
		count_n = count_n - 1;
		suspend;
	end
end!


set term ;!

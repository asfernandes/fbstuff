FbException = class(Exception)
public
	constructor create(status: Status);
	destructor destroy; override;

	function getStatus: Status;

	class procedure checkException(status: Status);
	class procedure catchException(status: Status; e: Exception);

private
	status: Status;
end;

ISC_DATE = Integer;
ISC_TIME = Integer;

ISC_QUAD = record
	high: Integer;
	low: Cardinal;
end;

{ FIXME: }

PerformanceInfo = record
end;

dsc = record
end;


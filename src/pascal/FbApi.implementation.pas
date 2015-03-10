constructor FbException.create(status: Status);
begin
	self.status := status.clone();
end;

destructor FbException.destroy;
begin
	status.dispose();
end;

function FbException.getStatus: Status;
begin
	Result := status;
end;

class procedure FbException.checkException(status: Status);
begin
	if ((status.getState() and Status.STATE_ERRORS) <> 0) then
		raise FbException.create(status);
end;

class procedure FbException.catchException(status: Status; e: Exception);
var
	statusVector: array[0..4] of NativeIntPtr;
	msg: AnsiString;
begin
	if (e.inheritsFrom(FbException)) then
		status.setErrors(FbException(e).getStatus().getErrors())
	else
	begin
		msg := e.message;

		statusVector[0] := NativeIntPtr(1);			// isc_arg_gds
		statusVector[1] := NativeIntPtr(335544382);	// isc_random
		statusVector[2] := NativeIntPtr(2);			// isc_arg_string
		statusVector[3] := NativeIntPtr(PAnsiChar(msg));
		statusVector[4] := NativeIntPtr(0);			// isc_arg_end

		status.setErrors(@statusVector);
	end
end;

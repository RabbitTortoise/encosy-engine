module;

export module EncosyCore.SharedBetweenManagers;

export
class SharedBetweenManagers
{
public:
	SharedBetweenManagers() {};
	~SharedBetweenManagers() {};

	size_t SignalCompositionChange()
	{
		CurrentWorldCompositionNumber_++;
		return CurrentWorldCompositionNumber_;
	}

	bool IsCurrentComposition(const size_t& number) const
	{
		return (number == CurrentWorldCompositionNumber_);
	}

	size_t GetCurrentComposition() const
	{
		return CurrentWorldCompositionNumber_;
	}

private:
	// Used mainly to signal other managers to rebuild critical structures
	size_t CurrentWorldCompositionNumber_ = 0;
};
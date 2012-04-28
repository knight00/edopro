--Mara of the Nordic Alfar
function c73417207.initial_effect(c)
	--synchro custom
	local e1=Effect.CreateEffect(c)
	e1:SetType(EFFECT_TYPE_SINGLE)
	e1:SetCode(EFFECT_SYNCHRO_MATERIAL_CUSTOM)
	e1:SetProperty(EFFECT_FLAG_CANNOT_DISABLE+EFFECT_FLAG_UNCOPYABLE)
	e1:SetTarget(c73417207.target)
	e1:SetValue(1)
	e1:SetOperation(c73417207.operation)
	c:RegisterEffect(e1)
end
c73417207.tuner_filter=aux.FALSE
function c73417207.filter(c,syncard,f)
	return c:IsSetCard(0x42) and c:IsNotTuner() and c:IsCanBeSynchroMaterial(syncard) and (f==nil or f(c))
end
function c73417207.target(e,syncard,f,minc,maxc)
	if minc>2 or maxc<2 then return false end
	local lv=syncard:GetLevel()-e:GetHandler():GetLevel()
	if lv<=0 then return false end
	local g=Duel.GetMatchingGroup(c73417207.filter,syncard:GetControler(),LOCATION_HAND,0,nil,syncard,f)
	return g:CheckWithSumEqual(Card.GetSynchroLevel,lv,2,2,syncard)
end
function c73417207.operation(e,tp,eg,ep,ev,re,r,rp,syncard,f,minc,maxc)
	local lv=syncard:GetLevel()-e:GetHandler():GetLevel()
	local g=Duel.GetMatchingGroup(c73417207.synfilter,syncard:GetControler(),LOCATION_HAND,LOCATION_HAND,c,syncard,f)
	local sg=g:SelectWithSumEqual(tp,Card.GetSynchroLevel,lv,2,2,syncard)
	Duel.SetSynchroMaterial(sg)
end

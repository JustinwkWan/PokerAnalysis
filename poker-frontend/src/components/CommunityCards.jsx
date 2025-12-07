const Card = ({ rank, suit }) => {
  const getSuitClass = (s) => {
    if (s === '♥' || s === 'Hearts') return 'hearts';
    if (s === '♦' || s === 'Diamonds') return 'diamonds';
    if (s === '♣' || s === 'Clubs') return 'clubs';
    if (s === '♠' || s === 'Spades') return 'spades';
    return '';
  };

  return (
    <div className={`card ${getSuitClass(suit)}`}>
      {rank}{suit}
    </div>
  );
};

const CommunityCards = ({ cards }) => {
  return (
    <div id="communityCards">
      <h3>Community Cards</h3>
      <div id="communityCardsDisplay">
        {cards && cards.length > 0 ? (
          cards.map((card, i) => (
            <Card key={i} rank={card.rank} suit={card.suit} />
          ))
        ) : null}
      </div>
    </div>
  );
};

export default CommunityCards;
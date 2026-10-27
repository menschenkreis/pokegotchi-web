export const versions = {
  forest: {name:'Forest', storageKey:'pokegotchi-browser-forest-v1', module:'./versions/forest/game.mjs', day:'media/forest/mosslet.png', night:'media/forest/night.png'},
  classic: {name:'Classic', storageKey:'pokegotchi-browser-v1', module:'./versions/classic/game.mjs', day:'media/day.png', night:'media/night.png'}
};
export function selectVersion(search){return new URLSearchParams(search).get('version')==='classic'?'classic':'forest';}

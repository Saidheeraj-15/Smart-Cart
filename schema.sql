-- SmartCart schema. Run in Supabase: SQL Editor > New query > Run.

create table stores (id text primary key, name text not null);

create table products (
  barcode text primary key,
  name text not null,
  brand text,
  image_url text,
  quantity text,
  ingredients text,
  allergens text,
  nutrients jsonb default '{}'::jsonb,   -- per 100 g/ml: energy_kcal, fat, sat_fat, carbs, sugars, fibre, protein, salt, fvl_pct
  nutriscore text check (nutriscore in ('a','b','c','d','e')),
  nova int check (nova between 1 and 4),
  category text default 'food',          -- use 'beverage' for drinks (calculator skips them)
  source text default 'manual',          -- 'off' or 'manual'
  verified boolean default false,
  created_at timestamptz default now()
);

create table store_products (
  store_id text references stores(id),
  barcode text references products(barcode),
  price numeric, mrp numeric, stock int default 0,
  aisle text, shelf text,
  primary key (store_id, barcode)
);

create table carts (id text primary key, store_id text references stores(id));

create table scans (
  id bigint generated always as identity primary key,
  cart_id text not null,
  barcode text not null,
  created_at timestamptz default now()
);

-- Row level security (prototype rules)
alter table stores enable row level security;
alter table products enable row level security;
alter table store_products enable row level security;
alter table carts enable row level security;
alter table scans enable row level security;

create policy "read stores" on stores for select using (true);
create policy "read products" on products for select using (true);
create policy "add products" on products for insert with check (true);  -- lets the app save Open Food Facts lookups
create policy "read store_products" on store_products for select using (true);
create policy "read carts" on carts for select using (true);
create policy "read scans" on scans for select using (true);
create policy "add scans" on scans for insert with check (true);        -- ESP32 inserts here

-- Realtime for scans
alter publication supabase_realtime add table scans;

-- Starter rows
insert into stores values ('S1', 'Demo Supermarket');
insert into carts values ('CART01', 'S1');

-- EXAMPLE ONLY: replace with your 10 real products
insert into products (barcode, name, brand, quantity, ingredients, allergens, nutrients, source, verified)
values ('8901234567890', 'Example Biscuits', 'ExampleBrand', '100 g',
        'Wheat flour, sugar, palm oil, invert syrup, raising agents (INS 500), emulsifier (INS 322), artificial flavour',
        'Gluten',
        '{"energy_kcal":480,"fat":18,"sat_fat":8.5,"carbs":71,"sugars":22,"fibre":2.1,"protein":6.5,"salt":0.9}',
        'manual', false);
insert into store_products values ('S1', '8901234567890', 10, 10, 50, 'A3', 'Shelf 2');
